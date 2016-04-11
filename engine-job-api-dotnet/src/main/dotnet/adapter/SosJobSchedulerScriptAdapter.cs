namespace com.sosberlin.jobscheduler.dotnet.adapter
{
    using sos.spooler;

    public abstract class SosJobSchedulerScriptAdapter
    {
        public string Script { get; set; }
        
        public Spooler spooler { get; private set; }
        public Job spooler_job { get; private set; }
        public Task spooler_task { get; private set; }
        public Log spooler_log { get; private set; }

        protected SosJobSchedulerScriptAdapter(Spooler contextSpooler, Job contextJob, Task contextTask, Log contextLog)
        {
            this.spooler = contextSpooler;
            this.spooler_job = contextJob;
            this.spooler_task = contextTask;
            this.spooler_log = contextLog;
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

        public void SetScript(string val)
        {
            this.Script = val;
        }
    }
}
