namespace sos.spooler
{
    using System;
    using System.Collections.Generic;
    using System.Collections.ObjectModel;
    using System.Globalization;
    using System.Linq;
    using System.Management.Automation;

    public class PowershellAdapter : ScriptAdapter
    {
        private bool isShellMode;
        private readonly PowerShell shell;

        #region Constructor

        public PowershellAdapter(
            Spooler contextSpooler, Job contextJob, Task contextTask, Log contextLog, String scriptContent)
            : base(contextSpooler, contextJob, contextTask, contextLog, scriptContent)
        {
            this.shell = PowerShell.Create();

            this.shell.Runspace.SessionStateProxy.SetVariable("spooler", this.spooler);
            this.shell.Runspace.SessionStateProxy.SetVariable("spooler_job", this.spooler_job);
            this.shell.Runspace.SessionStateProxy.SetVariable("spooler_task", this.spooler_task);
            this.shell.Runspace.SessionStateProxy.SetVariable("spooler_log", this.spooler_log);

            this.ParseScript();
        }

        #endregion

        #region Public override methods

        public override bool spooler_init()
        {
            if (this.isShellMode)
            {
                return true;
            }

            if (!this.InitializeScript())
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

        public override bool spooler_task_before()
        {
            if (!this.InitializeScript())
            {
                return false;
            }

            var results = this.InvokeCommand("spooler_task_before");
            var index = GetReturnValueIndex(results);
            this.Log(results, index);
            return GetReturnValue(results, index, true);
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

        public override bool spooler_process_before()
        {
            var results = this.InvokeCommand("spooler_process_before");
            var index = GetReturnValueIndex(results);
            this.Log(results, index);
            return GetReturnValue(results, index, true);
        }

        public override bool spooler_process()
        {
            var defaultReturnValue = this.spooler_task.order() != null;
            if (!this.isShellMode)
            {
                var results = this.InvokeCommand("spooler_process");
                var index = GetReturnValueIndex(results);
                this.Log(results, index);

                return GetReturnValue(results, index, defaultReturnValue);
            }

            this.InitializeScript();
            return defaultReturnValue;
        }

        public override bool spooler_process_after(bool spoolerProcessResult)
        {
            var results = this.InvokeCommand("spooler_process_after", spoolerProcessResult);
            var index = GetReturnValueIndex(results);
            this.Log(results, index);
            return GetReturnValue(results, index, true);
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

        public override void spooler_close()
        {
            if (this.isShellMode)
            {
                return;
            }

            var results = this.InvokeCommand("spooler_close");
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
                    t => t.Content.Equals("spooler_process") && t.Type.Equals(PSTokenType.CommandArgument));
            this.isShellMode = functionSpoolerProcess == null;
        }

        private bool InitializeScript()
        {
            this.shell.Commands.Clear();
            this.shell.AddScript(this.Script, this.isShellMode);
            this.shell.AddCommand("Out-String").AddParameter("Stream",true);
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
                this.spooler_log.error(this.shell.Streams.Error[0].ToString());
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
