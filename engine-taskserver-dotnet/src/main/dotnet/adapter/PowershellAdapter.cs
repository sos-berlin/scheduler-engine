namespace sos.spooler
{
    using System;
    using System.Collections.Generic;
    using System.Collections.ObjectModel;
    using System.Linq;
    using System.Management.Automation;
    using System.Management.Automation.Runspaces;
    using System.Text;

    public class PowershellAdapter : ScriptAdapter
    {
        #region Constants and Fields

        private PowershellAdapterPSHost host;
        private bool isShellMode;
        private Runspace runspace;
        private PowershellSpoolerParams spoolerParams;

        #endregion

        #region Constructors and Destructors

        public PowershellAdapter(
            Log contextLog, Task contextTask, Job contextJob, Spooler contextSpooler, String scriptContent)
            : base(contextLog, contextTask, contextJob, contextSpooler, scriptContent)
        {
            ParseScript();
            spoolerParams = new PowershellSpoolerParams(spooler_task, spooler, IsOrderJob, isShellMode);

            host = new PowershellAdapterPSHost(spooler_task, spooler_log);
            runspace = RunspaceFactory.CreateRunspace(host);
            runspace.Open();
            runspace.SessionStateProxy.SetVariable("spooler_log", spooler_log);
            runspace.SessionStateProxy.SetVariable("spooler_task", spooler_task);
            runspace.SessionStateProxy.SetVariable("spooler_job", spooler_job);
            runspace.SessionStateProxy.SetVariable("spooler", spooler);
            runspace.SessionStateProxy.SetVariable("spooler_params", spoolerParams);
        }

        #endregion

        #region Public Methods

        public override void spooler_close()
        {
            if (isShellMode)
            {
                return;
            }

            try
            {
                InvokeFunction("spooler_close");
            }
            catch (RuntimeException ex)
            {
                throw new Exception(GetErrorMessage("spooler_close", ex.ErrorRecord));
            }
        }

        public override void spooler_exit()
        {
            try
            {
                if (isShellMode)
                {
                }
                else
                {
                    InvokeFunction("spooler_exit");
                }
            }
            catch (RuntimeException ex)
            {
                throw new Exception(GetErrorMessage("spooler_exit", ex.ErrorRecord));
            }
            finally
            {
                Close();
            }
        }

        public override bool spooler_init()
        {
            if (isShellMode)
            {
                return true;
            }

            try
            {
                InitializeScript(false);
                var result = InvokeFunction("spooler_init");
                return GetReturnValue(result, true);
            }
            catch (RuntimeException ex)
            {
                throw new Exception(GetErrorMessage("spooler_init", ex.ErrorRecord));
            }
        }

        public override void spooler_on_error()
        {
            if (isShellMode)
            {
                return;
            }
            try
            {
                InvokeFunction("spooler_on_error");
            }
            catch (RuntimeException ex)
            {
                throw new Exception(GetErrorMessage("spooler_on_error", ex.ErrorRecord));
            }
        }

        public override void spooler_on_success()
        {
            if (isShellMode)
            {
                return;
            }
            try
            {
                InvokeFunction("spooler_on_success");
            }
            catch (RuntimeException ex)
            {
                throw new Exception(GetErrorMessage("spooler_on_success", ex.ErrorRecord));
            }
        }

        public override bool spooler_open()
        {
            if (isShellMode)
            {
                return true;
            }

            try
            {
                var result = InvokeFunction("spooler_open");
                return GetReturnValue(result, true);
            }
            catch (RuntimeException ex)
            {
                throw new Exception(GetErrorMessage("spooler_open", ex.ErrorRecord));
            }
        }

        public override bool spooler_process()
        {
            try
            {
                if (isShellMode)
                {
                    spoolerParams.SetEnvVars();
                    InitializeScript(true);
                    HandleGlobalLastExitCode(true);
                    return IsOrderJob;
                }

                var result = InvokeFunction("spooler_process");
                return GetReturnValue(result, IsOrderJob);
            }
            catch (RuntimeException ex)
            {
                throw new Exception(GetErrorMessage(isShellMode ? "" : "spooler_process", ex.ErrorRecord));
            }
        }

        public override bool spooler_process_after(bool spoolerProcessResult)
        {
            try
            {
                var result = InvokeFunction("spooler_process_after", spoolerProcessResult);
                return GetReturnValue(result, spoolerProcessResult);
            }
            catch (RuntimeException ex)
            {
                throw new Exception(GetErrorMessage("spooler_process_after", ex.ErrorRecord));
            }
        }

        public override bool spooler_process_before()
        {
            try
            {
                var result = InvokeFunction("spooler_process_before");
                return GetReturnValue(result, true);
            }
            catch (RuntimeException ex)
            {
                throw new Exception(GetErrorMessage("spooler_process_before", ex.ErrorRecord));
            }
        }

        public override void spooler_task_after()
        {
            try
            {
                InvokeFunction("spooler_task_after");
            }
            catch (RuntimeException ex)
            {
                throw new Exception(GetErrorMessage("spooler_task_after", ex.ErrorRecord));
            }
            finally
            {
                Close();
            }
        }

        public override bool spooler_task_before()
        {
            try
            {
                InitializeScript(false);
                var result = InvokeFunction("spooler_task_before");
                return GetReturnValue(result, true);
            }
            catch (RuntimeException ex)
            {
                throw new Exception(GetErrorMessage("spooler_task_before", ex.ErrorRecord));
            }
        }

        #endregion

        #region Methods

        private static string GetErrorMessage(String functionName, ErrorRecord errorRecord)
        {
            var sb = new StringBuilder();
            if (!String.IsNullOrEmpty(functionName))
            {
                sb.Append(String.Format("[{0}] ", functionName));
            }
            sb.Append(errorRecord.ToString());
            sb.Append(errorRecord.InvocationInfo.PositionMessage);
            return sb.ToString();
        }

        private static bool GetReturnValue(string result, bool defaultValue)
        {
            var rs = defaultValue;
            if (result != null)
            {
                try
                {
                    rs = Boolean.Parse(result);
                }
                catch (Exception)
                {
                }
            }
            return rs;
        }

        private int GetGlobalLastExitCode(bool useLocalScope)
        {
            var lastExitCode = InvokeCommand(useLocalScope, "$Global:LastExitCode").FirstOrDefault();
            var exitCode = 0;
            if (lastExitCode != null)
            {
                try
                {
                    exitCode = Int32.Parse(lastExitCode.ToString());
                }
                catch (Exception)
                {
                }
            }
            return exitCode;
        }

        private void Close()
        {
            runspace.Close();
            runspace.Dispose();
            
            runspace = null;
            host = null;
            spoolerParams = null;
        }

        private void HandleGlobalLastExitCode(bool useLocalScope, String functionName = "")
        {
            int exitCode = GetGlobalLastExitCode(useLocalScope);
            if (exitCode != 0)
            {
                host.SetShouldExit(exitCode);
            }
            host.LastExitCode = exitCode;
        }

        private void InitializeScript(bool useLocalScope)
        {
            InvokeScript(useLocalScope, Script);
        }

        private IEnumerable<PSObject> InvokeCommand(bool useLocalScope, String command)
        {
            Collection<PSObject> result;
            using (var pipeline = runspace.CreatePipeline())
            {
                pipeline.Commands.AddScript(command, useLocalScope);
                result = pipeline.Invoke();
            }
            return result;
        }

        private string InvokeFunction(String functionName, bool? param = null)
        {
            ((PowershellAdapterPSHostUserInterface)host.UI).CurrentFunctionName = functionName;

            var functionParams = "";
            if (param.HasValue)
            {
                functionParams = "($" + param.Value + ")";
            }

            var command = String.Format(
                "if($function:{0}){{ {1}{2} }}",
                functionName,
                functionName,
                functionParams);

            var result = InvokeScript(false, command);
            HandleGlobalLastExitCode(false, functionName);
            return result;
        }

        private string InvokeScript(bool useLocalScope, String command)
        {
            using (var pipeline = runspace.CreatePipeline())
            {
                pipeline.Commands.AddScript(command, useLocalScope);
                pipeline.Commands.Add("Out-Default");
                pipeline.Commands[0].MergeMyResults(PipelineResultTypes.Error, PipelineResultTypes.Output);
                pipeline.Invoke();
            }
            return ((PowershellAdapterPSHostUserInterface)host.UI).LastInfoMessage;
        }

        private void ParseScript()
        {
            if (string.IsNullOrEmpty(Script))
            {
                throw new Exception("Script is null or empty.");
            }

            Collection<PSParseError> parseErrors;
            var tokens = PSParser.Tokenize(Script, out parseErrors);
            var apiFunction =
                tokens.FirstOrDefault(
                    t => t.Type.Equals(PSTokenType.CommandArgument) &&
                         (t.Content.Equals("spooler_init")
                          || t.Content.Equals("spooler_open")
                          || t.Content.Equals("spooler_process")
                          || t.Content.Equals("spooler_close")
                          || t.Content.Equals("spooler_on_success")
                          || t.Content.Equals("spooler_on_error")
                          || t.Content.Equals("spooler_exit")));
            isShellMode = apiFunction == null;
        }

        #endregion
    }
}
