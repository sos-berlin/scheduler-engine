namespace sos.spooler
{
    using System;
   
    public class Monitor_impl
    {
        public Spooler spooler { get; private set; }
        public Job spooler_job { get; private set; }
        public Task spooler_task { get; private set; }
        public Log spooler_log { get; private set; }

        protected Monitor_impl()
        {
        }

        protected Monitor_impl(Spooler contextSpooler, Job contextJob, Task contextTask, Log contextLog)
        {
            this.spooler = contextSpooler;
            this.spooler_job = contextJob;
            this.spooler_task = contextTask;
            this.spooler_log = contextLog;
        }

        public virtual bool spooler_task_before()
        {
            return true;
        }

        public virtual void spooler_task_after()
        {
        }

        public virtual bool spooler_process_before()
        {
            return true;
        }

        public virtual bool spooler_process_after(bool spoolerProcessResult)
        {
            return spoolerProcessResult;
        }

        public bool ToBoolean(string value)
        {
            return Boolean.Parse(value);
        }
       
    }
}
