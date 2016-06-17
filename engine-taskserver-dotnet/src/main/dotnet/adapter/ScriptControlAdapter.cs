namespace sos.spooler
{
    using System;

    public class ScriptControlAdapter : ScriptAdapter
    {
        private readonly dynamic scriptControl;

        #region Constructor

        public ScriptControlAdapter(
            Log contextLog, Task contextTask, Job contextJob, Spooler contextSpooler, string scriptContent,
            string language)
            : base(contextLog, contextTask, contextJob, contextSpooler, scriptContent)
        {
            var scriptType = Type.GetTypeFromCLSID(Guid.Parse("0E59F1D5-1FBE-11D0-8FF2-00A0D10038BC"));
            this.scriptControl = Activator.CreateInstance(scriptType, false);
            this.scriptControl.Language = language;

            this.scriptControl.AddObject("spooler_log", this.spooler_log, true);
            this.scriptControl.AddObject("spooler_task", this.spooler_task, true);
            this.scriptControl.AddObject("spooler_job", this.spooler_job, true);
            this.scriptControl.AddObject("spooler", this.spooler, true);

            this.scriptControl.AddCode(scriptContent);
        }

        #endregion

        #region Public JobScheduler API methods

        #region Public Job_impl methods

        public override bool spooler_init()
        {
            var result = this.scriptControl.Eval("spooler_init");
            return GetReturnValue(result, true);
        }

        public override bool spooler_open()
        {
            var result = this.scriptControl.Eval("spooler_open");
            return GetReturnValue(result, true);
        }

        public override bool spooler_process()
        {
            var result = this.scriptControl.Eval("spooler_process");
            return GetReturnValue(result, this.IsOrderJob);
        }

        public override void spooler_close()
        {
            this.scriptControl.Eval("spooler_close");
        }

        public override void spooler_on_success()
        {
            this.scriptControl.Eval("spooler_on_success");
        }

        public override void spooler_on_error()
        {
            this.scriptControl.Eval("spooler_on_error");
        }

        public override void spooler_exit()
        {
            this.scriptControl.Eval("spooler_exit");
        }

        #endregion

        #region Public Monitor_impl methods

        public override bool spooler_task_before()
        {
            var result = this.scriptControl.Eval("spooler_task_before");
            return GetReturnValue(result, true);
        }

        public override bool spooler_process_before()
        {
            var result = this.scriptControl.Eval("spooler_process_before");
            return GetReturnValue(result, true);
        }

        public override bool spooler_process_after(bool spoolerProcessResult)
        {
            var result = this.scriptControl.Eval("spooler_process_after(" + spoolerProcessResult + ")");
            return GetReturnValue(result, spoolerProcessResult);
        }

        public override void spooler_task_after()
        {
            this.scriptControl.Eval("spooler_task_after");
        }

        #endregion

        #endregion

        #region Private methods

        private static bool GetReturnValue(dynamic value, bool defaultValue)
        {
            return value != null && value is Boolean ? value : defaultValue;
        }

        #endregion
    }
}
