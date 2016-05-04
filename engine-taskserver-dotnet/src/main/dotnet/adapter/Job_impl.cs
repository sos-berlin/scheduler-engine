namespace sos.spooler
{
    public class Job_impl
    {
        public Spooler spooler { get; private set; }
        public Job spooler_job { get; private set; }
        public Task spooler_task { get; private set; }
        public Log spooler_log { get; private set; }

        public Job_impl(Spooler contextSpooler, Job contextJob, Task contextTask, Log contextLog)
        {
            this.spooler = contextSpooler;
            this.spooler_job = contextJob;
            this.spooler_task = contextTask;
            this.spooler_log = contextLog;
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
    }
}
