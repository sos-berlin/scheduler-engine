namespace sos.spooler
{
    using System;

    public abstract class ScriptAdapter
    {
        public Spooler spooler { get; private set; }
        public Job spooler_job { get; private set; }
        public Task spooler_task { get; private set; }
        public Log spooler_log { get; private set; }
        protected string Script { get; private set; }
        
        protected ScriptAdapter(Spooler contextSpooler, Job contextJob, Task contextTask, Log contextLog, String scriptContent)
        {
            this.spooler = contextSpooler;
            this.spooler_job = contextJob;
            this.spooler_task = contextTask;
            this.spooler_log = contextLog;
            this.Script = scriptContent;
        }
      
        public abstract bool spooler_task_before();
        public abstract void spooler_task_after();
        public abstract bool spooler_process_before();
        public abstract bool spooler_process_after(bool spoolerProcessResult);
        public abstract bool spooler_init();
        public abstract bool spooler_open();
        public abstract void spooler_close();
        public abstract void spooler_on_success();
        public abstract void spooler_on_error();
        public abstract bool spooler_process();
        public abstract void spooler_exit();
        public bool ToBoolean(string value)
        {
            return Boolean.Parse(value);
        }
    }
}
