namespace sos.spooler
{
    using System;
    using System.Collections.Generic;
    using System.Collections.ObjectModel;
    using System.Globalization;
    using System.Linq;
    using System.Management.Automation;
    using System.Text;

    public class PowershellAdapter : ScriptAdapter
    {
        private bool isShellMode;
        private readonly PowershellSpoolerParams spoolerParams;
        private readonly PowerShell shell;
        
        #region Constructor

        public PowershellAdapter(
            Log contextLog, Task contextTask, Job contextJob, Spooler contextSpooler, String scriptContent)
            : base(contextLog, contextTask, contextJob, contextSpooler, scriptContent)
        {
            this.ParseScript();
            this.spoolerParams = new PowershellSpoolerParams(this.spooler_task, this.spooler, this.IsOrderJob, this.isShellMode);

            this.shell = PowerShell.Create();
            this.shell.Runspace.SessionStateProxy.SetVariable("spooler_log", this.spooler_log);
            this.shell.Runspace.SessionStateProxy.SetVariable("spooler_task", this.spooler_task);
            this.shell.Runspace.SessionStateProxy.SetVariable("spooler_job", this.spooler_job);
            this.shell.Runspace.SessionStateProxy.SetVariable("spooler", this.spooler);
            this.shell.Runspace.SessionStateProxy.SetVariable("spooler_params", this.spoolerParams);
        }

        #endregion

        #region Public JobScheduler API methods

        #region Public Job_impl methods

        public override bool spooler_init()
        {
            if (this.isShellMode)
            {
                return true;
            }

            if (!this.InitializeScript(false))
            {
                return false;
            }

            var results = this.InvokeCommand("spooler_init");
            var index = GetReturnValueIndex(results);
            this.Log(results, index);
            return GetReturnValue(results, index, true);
        }

        public override bool spooler_open()
        {
            if (this.isShellMode)
            {
                return true;
            }

            var results = this.InvokeCommand("spooler_open");
            var index = GetReturnValueIndex(results);
            this.Log(results, index);
            return GetReturnValue(results, index, true);
        }

        public override bool spooler_process()
        {
            if (this.isShellMode)
            {
                this.spoolerParams.SetEnvVars();
                this.InitializeScript(true);
                return this.IsOrderJob;
            }

            var results = this.InvokeCommand("spooler_process");
            var index = GetReturnValueIndex(results);
            this.Log(results, index);
            return GetReturnValue(results, index, this.IsOrderJob);
        }

        public override void spooler_close()
        {
            if (this.isShellMode)
            {
                return;
            }

            var results = this.InvokeCommand("spooler_close");
            this.Log(results);
        }

        public override void spooler_on_success()
        {
            if (this.isShellMode)
            {
                return;
            }

            var results = this.InvokeCommand("spooler_on_success");
            this.Log(results);
        }

        public override void spooler_on_error()
        {
            if (this.isShellMode)
            {
                return;
            }

            var results = this.InvokeCommand("spooler_on_error");
            this.Log(results);
        }

        public override void spooler_exit()
        {
            try
            {
                if (this.isShellMode)
                {
                }
                else
                {
                    var results = this.InvokeCommand("spooler_exit");
                    this.Log(results);
                }
            }
            finally
            {
                this.Close();
            }
        }

        #endregion

        #region Public Monitor_impl methods

        public override bool spooler_task_before()
        {
            if (!this.InitializeScript(false))
            {
                return false;
            }

            var results = this.InvokeCommand("spooler_task_before");
            var index = GetReturnValueIndex(results);
            this.Log(results, index);
            return GetReturnValue(results, index, true);
        }

        public override bool spooler_process_before()
        {
            var results = this.InvokeCommand("spooler_process_before");
            var index = GetReturnValueIndex(results);
            this.Log(results, index);
            return GetReturnValue(results, index, true);
        }

        public override bool spooler_process_after(bool spoolerProcessResult)
        {
            var results = this.InvokeCommand("spooler_process_after", spoolerProcessResult);
            var index = GetReturnValueIndex(results);
            this.Log(results, index);
            return GetReturnValue(results, index, spoolerProcessResult);
        }

        public override void spooler_task_after()
        {
            try
            {
                var results = this.InvokeCommand("spooler_task_after");
                this.Log(results);
            }
            finally
            {
                this.Close();
            }
        }

        #endregion

        #endregion

        #region Private methods

        private void ParseScript()
        {
            if (string.IsNullOrEmpty(this.Script))
            {
                throw new Exception("Script is null or empty.");
            }

            Collection<PSParseError> parseErrors;
            var tokens = PSParser.Tokenize(this.Script, out parseErrors);
            var functionSpoolerProcess =
                tokens.FirstOrDefault(
                    t => t.Type.Equals(PSTokenType.CommandArgument) &&
                    (t.Content.Equals("spooler_init")
                    || t.Content.Equals("spooler_open")
                    || t.Content.Equals("spooler_process")
                    || t.Content.Equals("spooler_close")
                    || t.Content.Equals("spooler_on_success")
                    || t.Content.Equals("spooler_on_error")
                    || t.Content.Equals("spooler_exit")));
            this.isShellMode = functionSpoolerProcess == null;
        }

        private bool InitializeScript(bool useGlobalScope)
        {
            this.shell.Commands.Clear();
            this.shell.AddScript(this.Script, useGlobalScope);
            this.shell.AddCommand("Out-String").AddParameter("Stream", true);
            var results = this.shell.Invoke();
            var success = this.shell.Streams.Error.Count == 0;
            this.Log(results);

            return success;
        }

        private Collection<PSObject> InvokeCommand(String methodName, bool? param = null)
        {
            this.shell.Commands.Clear();
            var methodParams = "";
            if (param.HasValue)
            {
                var str = string.Concat("$", param.Value.ToString(CultureInfo.InvariantCulture));
                methodParams = "(" + str + ")";
            }
            this.shell.AddScript(
                "if ($function:" + methodName + ") {" + methodName + methodParams + " | Out-String -Stream}", false);
            return this.shell.Invoke();
        }

        private void Log(IEnumerable<PSObject> results, int returnValueIndex = -1)
        {
            this.LogResults(results, returnValueIndex);
            this.LogStreams();
        }

        private void LogResults(IEnumerable<PSObject> results, int returnValueIndex)
        {
            if (results == null)
            {
                return;
            }
            var i = 0;
            foreach (var psObject in results)
            {
                if (i != returnValueIndex)
                {
                    this.spooler_log.info(psObject.ToString());
                }
                i++;
            }
        }

        private void LogStreams()
        {
            if (this.shell.Streams.Error.Count > 0)
            {
                this.spooler_log.error(GetErrorMessage(this.shell.Streams.Error));
            }
            if (this.shell.Streams.Warning.Count > 0)
            {
                this.spooler_log.warn(this.shell.Streams.Warning[0].ToString());
            }
            if (this.shell.Streams.Debug.Count > 0)
            {
                this.spooler_log.debug1(this.shell.Streams.Debug[0].ToString());
            }
            if (this.shell.Streams.Verbose.Count > 0)
            {
                this.spooler_log.info(this.shell.Streams.Verbose[0].ToString());
            }
            this.shell.Streams.ClearStreams();
        }

        private static string GetErrorMessage(IList<ErrorRecord> rec)
        {
            var sb = new StringBuilder(rec[0].ToString());
            sb.Append(Environment.NewLine);
            sb.Append(
                String.Format(
                    "At line: {0} char: {1}. For: {2}"
                    , rec[0].InvocationInfo.ScriptLineNumber
                    , rec[0].InvocationInfo.OffsetInLine
                    , rec[0].InvocationInfo.Line));
            return sb.ToString();
        }

        private static int GetReturnValueIndex(IEnumerable<PSObject> results)
        {
            var index = -1;
            try
            {
                index = results.Select(
                    (v, i) => new
                    {
                        Value = v.ToString().ToLower(),
                        Index = i
                    }).Where(x => x.Value.Equals("true") || x.Value.Equals("false")).Select(x => x.Index).Last();
            }
            catch (Exception)
            {
            }
            return index;
        }

        private static bool GetReturnValue(IEnumerable<PSObject> results, int returnValueIndex, bool defaultValue)
        {
            var result = defaultValue;
            if (returnValueIndex > -1)
            {
                try
                {
                    result = Boolean.Parse(results.ElementAt(returnValueIndex).ToString());
                }
                catch (Exception)
                {
                }
            }
            return result;
        }

        private void Close()
        {
            this.shell.Dispose();
        }

        #endregion
    }
}
