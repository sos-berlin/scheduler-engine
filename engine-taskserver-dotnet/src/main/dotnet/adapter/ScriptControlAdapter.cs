namespace sos.spooler
{
    using System;
    using System.Text;

    public class ScriptControlAdapter : ScriptAdapter
    {
        private readonly SpoolerParams spoolerParams;
        private readonly dynamic scriptControl;

        #region Constructor

        public ScriptControlAdapter(
            Log contextLog, Task contextTask, Job contextJob, Spooler contextSpooler, string contextStdErrLogLevel, string scriptContent,
            string language)
            : base(contextLog, contextTask, contextJob, contextSpooler, contextStdErrLogLevel, scriptContent)
        {
            spoolerParams = new SpoolerParams(spooler_task, spooler, IsOrderJob, false);

            var scriptType = Type.GetTypeFromCLSID(Guid.Parse("0E59F1D5-1FBE-11D0-8FF2-00A0D10038BC"));
            this.scriptControl = Activator.CreateInstance(scriptType, false);
            this.scriptControl.Language = language;

            this.scriptControl.AddObject("spooler_log", spooler_log, false);
            this.scriptControl.AddObject("spooler_task", spooler_task, false);
            this.scriptControl.AddObject("spooler_job", spooler_job, false);
            this.scriptControl.AddObject("spooler", spooler, false);
            this.scriptControl.AddObject("spooler_params", spoolerParams, false);
        }

        #endregion

        #region Public JobScheduler API methods

        #region Public Job_impl methods

        public override bool spooler_init()
        {
            try
            {
                InitializeScript(false);

                var result = Eval("spooler_init");
                return GetReturnValue(result, true);
            }
            catch (Exception ex)
            {
                throw new Exception(GetErrorMessage(ex));
            }
        }

        public override bool spooler_open()
        {
            try
            {
                var result = Eval("spooler_open");
                return GetReturnValue(result, true);
            }
            catch (Exception ex)
            {
                throw new Exception(GetErrorMessage(ex));
            }
        }

        public override bool spooler_process()
        {
            try
            {
                var result = Eval("spooler_process");
                return GetReturnValue(result, IsOrderJob);
            }
            catch (Exception ex)
            {
                throw new Exception(GetErrorMessage(ex));
            }
        }

        public override void spooler_close()
        {
            try
            {
                Eval("spooler_close");
            }
            catch (Exception ex)
            {
                throw new Exception(GetErrorMessage(ex));
            }
        }

        public override void spooler_on_success()
        {
            try
            {
                Eval("spooler_on_success");
            }
            catch (Exception ex)
            {
                throw new Exception(GetErrorMessage(ex));
            }
        }

        public override void spooler_on_error()
        {
            try
            {
                Eval("spooler_on_error");
            }
            catch (Exception ex)
            {
                throw new Exception(GetErrorMessage(ex));
            }
        }

        public override void spooler_exit()
        {
            try
            {
                Eval("spooler_exit");
            }
            catch (Exception ex)
            {
                throw new Exception(GetErrorMessage(ex));
            }
        }

        #endregion

        #region Public Monitor_impl methods

        public override bool spooler_task_before()
        {
            try
            {
                InitializeScript(true);

                var result = Eval("spooler_task_before");
                return GetReturnValue(result, true);
            }
            catch (Exception ex)
            {
                throw new Exception(GetErrorMessage(ex));
            }
        }

        public override bool spooler_process_before()
        {
            try
            {
                var result = Eval("spooler_process_before");
                return GetReturnValue(result, true);
            }
            catch (Exception ex)
            {
                throw new Exception(GetErrorMessage(ex));
            }
        }

        public override bool spooler_process_after(bool spoolerProcessResult)
        {
            try
            {
                var result = Eval("spooler_process_after(" + spoolerProcessResult + ")");
                return GetReturnValue(result, spoolerProcessResult);
            }
            catch (Exception ex)
            {
                throw new Exception(GetErrorMessage(ex));
            }
        }

        public override void spooler_task_after()
        {
            try
            {
                Eval("spooler_task_after");
            }
            catch (Exception ex)
            {
                throw new Exception(GetErrorMessage(ex));
            }
        }

        #endregion

        #endregion

        #region Private methods

        private void InitializeScript(bool isMonitorImpl)
        {
            if (isMonitorImpl)
            {
                this.scriptControl.AddCode(GetMonitorImplDefaultFunctions());
            }
            this.scriptControl.AddCode(Script);
        }

        private object Eval(string expresion)
        {
            this.scriptControl.Error.Clear();
            return this.scriptControl.Eval(expresion);
        }

        private string GetErrorMessage(Exception ex)
        {
            return String.Format(
                "{0} {1}: {2}{3}Line: {4}, char: {5}{6}"
                , this.scriptControl.Error.Number
                , this.scriptControl.Error.Source
                , this.scriptControl.Error.Description
                , Environment.NewLine
                , this.scriptControl.Error.Line
                , this.scriptControl.Error.Column
                , this.scriptControl.Error.Text);
        }

        private static string GetMonitorImplDefaultFunctions()
        {
            var sb = new StringBuilder();
            sb.Append("Function spooler_process_after(spoolerProcessResult) ");
            sb.Append(Environment.NewLine);
            sb.Append("spooler_process_after=spoolerProcessResult");
            sb.Append(Environment.NewLine);
            sb.Append("End Function");
            return sb.ToString();
        }

        private static bool GetReturnValue(dynamic value, bool defaultValue)
        {
            return value != null && value is Boolean ? value : defaultValue;
        }

        #endregion
    }
}
