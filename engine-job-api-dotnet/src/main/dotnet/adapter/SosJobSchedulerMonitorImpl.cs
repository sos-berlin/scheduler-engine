namespace com.sosberlin.jobscheduler.dotnet.adapter
{
    using sos.spooler;
    using System;
   
    public class SosJobSchedulerMonitorImpl
    {
        public Spooler spooler { get; private set; }
        public Job spooler_job { get; private set; }
        public Task spooler_task { get; private set; }
        public Log spooler_log { get; private set; }

        protected SosJobSchedulerMonitorImpl(Spooler javaSpooler, Job javaJob, Task javaTask, Log javaLog)
        {
            this.spooler = javaSpooler;
            this.spooler_job = javaJob;
            this.spooler_task = javaTask;
            this.spooler_log = javaLog;
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
