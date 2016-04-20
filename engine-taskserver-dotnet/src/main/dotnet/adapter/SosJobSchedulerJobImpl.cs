namespace com.sosberlin.jobscheduler.dotnet.adapter
{
    using sos.spooler;

    public class SosJobSchedulerJobImpl
    {
        public Spooler spooler { get; private set; }
        public Job spooler_job { get; private set; }
        public Task spooler_task { get; private set; }
        public Log spooler_log { get; private set; }
        
        protected SosJobSchedulerJobImpl(Spooler javaSpooler, Job javaJob, Task javaTask, Log javaLog)
        {
            this.spooler = javaSpooler;
            this.spooler_job = javaJob;
            this.spooler_task = javaTask;
            this.spooler_log = javaLog;
        }

        public virtual bool spooler_init()
        {
            return true;
        }

        public virtual void spooler_exit()
        {
        }

        public virtual bool spooler_open()
        {
            return true;
        }

        public virtual void spooler_close()
        {
        }

        public virtual bool spooler_process()
        {
            return false;
        }

        public virtual void spooler_on_error()
        {
        }

        public virtual void spooler_on_success()
        {
        }

        public string spooler_api_version() {
		    return "2.0.160.4605 (2006-11-23)";
	    }
        
    }
}
