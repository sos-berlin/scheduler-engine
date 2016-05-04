namespace sos.spooler
{
    using System;
    using System.Collections.Generic;
    using System.Globalization;
    using System.Linq;
    using System.Management.Automation;

    public class PowershellAdapter : ScriptAdapter
    {
        private PowerShell Shell { get; set; }
     
        public PowershellAdapter(Spooler contextSpooler, Job contextJob, Task contextTask, Log contextLog,String scriptContent)
            : base(contextSpooler, contextJob, contextTask, contextLog, scriptContent)
        {
            this.Shell = PowerShell.Create();

            this.Shell.Runspace.SessionStateProxy.SetVariable("spooler", this.spooler);
            this.Shell.Runspace.SessionStateProxy.SetVariable("spooler_job", this.spooler_job);
            this.Shell.Runspace.SessionStateProxy.SetVariable("spooler_task", this.spooler_task);
            this.Shell.Runspace.SessionStateProxy.SetVariable("spooler_log", this.spooler_log);

            this.InitializeScript();
        }

        #region Public override
        public override bool spooler_task_before()
        {
            this.Shell.AddScript("if ($function:spooler_task_before) {spooler_task_before}");
            var result = this.Shell.Invoke();
            this.ProcessOutputStreams();
            return GetRetVal(result, true);
        }

        public override void spooler_task_after()
        {

            this.Shell.Commands.Clear();
            this.Shell.AddScript("if ($function:spooler_task_after) {spooler_task_after}");
            this.Shell.Invoke();
            this.ProcessOutputStreams();
            this.Shell.Dispose();
        }

        public override bool spooler_process_before()
        {
            this.Shell.Commands.Clear();
            this.Shell.AddScript("if ($function:spooler_process_before) {spooler_process_before}");
            var result = this.Shell.Invoke();
            this.ProcessOutputStreams();
            return GetRetVal(result, true);
        }

        public override bool spooler_process_after(bool spoolerProcessResult)
        {
            this.Shell.Commands.Clear();
            var str = string.Concat("$", spoolerProcessResult.ToString(CultureInfo.InvariantCulture));
            this.Shell.AddScript(string.Concat("if ($function:spooler_process_after) {spooler_process_after(", str, ")}"));
            var result = this.Shell.Invoke();
            this.ProcessOutputStreams();
            return GetRetVal(result, spoolerProcessResult);
        }

        public override bool spooler_init()
        {
            this.Shell.AddScript("if ($function:spooler_init) {spooler_init}");
            var result = this.Shell.Invoke();
            this.ProcessOutputStreams();
            return GetRetVal(result, true);
        }

        public override bool spooler_open()
        {
            this.Shell.Commands.Clear();
            this.Shell.AddScript("if ($function:spooler_open) {spooler_open}");
            var result = this.Shell.Invoke();
            this.ProcessOutputStreams();
            return GetRetVal(result, true);
        }

        public override void spooler_close()
        {
            this.Shell.Commands.Clear();
            this.Shell.AddScript("if ($function:spooler_close) {spooler_close}");
            this.Shell.Invoke();
            this.ProcessOutputStreams();
        }

        public override void spooler_on_success()
        {
            this.Shell.Commands.Clear();
            this.Shell.AddScript("if ($function:spooler_on_success) {spooler_on_success}");
            this.Shell.Invoke();
            this.ProcessOutputStreams();
        }

        public override void spooler_on_error()
        {
            this.Shell.Commands.Clear();
            this.Shell.AddScript("if ($function:spooler_on_error) {spooler_on_error}");
            this.Shell.Invoke();
            this.ProcessOutputStreams();
        }

        public override bool spooler_process()
        {
            this.Shell.Commands.Clear();
            this.Shell.AddScript("if ($function:spooler_process) {spooler_process}");
            var result = this.Shell.Invoke();
            this.ProcessOutputStreams();
            var defaultValue = this.spooler_task.order() != (dynamic)null;

            return  (bool)GetRetVal(result, defaultValue);
        }

        public override void spooler_exit()
        {
            this.Shell.Commands.Clear();
            this.Shell.AddScript("if ($function:spooler_exit) {spooler_exit}");
            this.Shell.Invoke();
            this.ProcessOutputStreams();
            this.Shell.Dispose();
        }
        #endregion

        #region Private
        private void InitializeScript()
        {
            if (string.IsNullOrEmpty(this.Script))
            {
                throw new Exception("Script is null or empty.");
            }
           
            this.Shell.AddScript(this.Script);
            this.Shell.Invoke();
            if (this.Shell.Streams.Error.Count <= 0)
            {
                this.Shell.Commands.Clear();
            }
            else
            {
                var err = this.Shell.Streams.Error[0].ToString();
                throw new Exception("Error parsing script: " + err);
            }
        }
        
        private void ProcessOutputStreams()
        {
            if (this.Shell.Streams.Error.Count > 0)
            {
                var txt = this.Shell.Streams.Error[0].ToString();
                this.spooler_log.error(string.Concat("[powershell stderr]:", txt));
            }
            if (this.Shell.Streams.Warning.Count > 0)
            {
                var txt = this.Shell.Streams.Warning[0].ToString();
                this.spooler_log.warn(string.Concat("[powershell warning]:", txt));
            }
            if (this.Shell.Streams.Debug.Count > 0)
            {
                var txt = this.Shell.Streams.Debug[0].ToString();
                this.spooler_log.debug1(string.Concat("[powershell debug]:", txt));
            }
            if (this.Shell.Streams.Verbose.Count > 0)
            {
                var txt = this.Shell.Streams.Verbose[0].ToString();
                this.spooler_log.info(string.Concat("[powershell verbose]:", txt));
            }
            this.Shell.Streams.ClearStreams();
        }

        private static bool GetRetVal(IEnumerable<PSObject> coll, bool defaultValue)
        {
            bool? baseObject = null;
            try
            {
                baseObject = coll.First(p => p.TypeNames.Contains("System.Boolean")).BaseObject as bool?;
            }
            catch (InvalidOperationException)
            {
            }
            return baseObject.HasValue ? baseObject.Value : defaultValue;
        }

        #endregion
    }
}
