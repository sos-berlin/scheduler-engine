namespace sos.spooler
{
    using System;

    public class SpoolerParams
    {
        private readonly Task spoolerTask;
        private readonly Spooler spooler;
        private readonly bool isOrderJob;
        private readonly bool isShellMode;

        private readonly string schedulerVariableNamePrefix;

        #region Constructor

        public SpoolerParams(Task task, Spooler spooler, bool isOrderJob, bool isShellMode)
        {
            this.spoolerTask = task;
            this.spooler = spooler;
            this.isOrderJob = isOrderJob;
            this.isShellMode = isShellMode;
            if (this.isShellMode)
            {
                this.schedulerVariableNamePrefix = this.spooler.variables().value("scheduler.variable_name_prefix");
            }
        }

        #endregion

        #region Public Methods

        public Variable_set getAll()
        {
            var parameters = this.spooler.create_variable_set();
            parameters.merge(this.spoolerTask.@params());
            if (this.isOrderJob)
            {
                var op = this.spoolerTask.order();
                if (op != null)
                {
                    parameters.merge(this.spoolerTask.order().@params());
                }
            }
            return parameters;
        }

        public string get(string name)
        {
            string result = null;
            if (this.isOrderJob)
            {
                var op = this.spoolerTask.order();
                if (op != null)
                {
                    result = this.spoolerTask.order().@params().var(name);
                }
            }
            return result ?? (this.spoolerTask.@params().var(name));
        }

        public string value(string name)
        {
            return this.get(name);
        }

        public void set(string name, string value)
        {
            if (this.isOrderJob)
            {
                var op = this.spoolerTask.order();
                if (op != null)
                {
                    this.spoolerTask.order().@params().set_var(name, value);
                }
            }
            else
            {
                this.spoolerTask.@params().set_var(name, value);
            }

            if (this.isShellMode)
            {
                this.SetEnvVar(name, value);
            }
        }

        #endregion

        #region Internal methods

        internal void SetEnvVars()
        {
            if (!this.isShellMode)
            {
                return;
            }

            var parameters = this.getAll();
            var names = parameters.names().Split(';');
            foreach (var name in names)
            {
                this.SetEnvVar(name, parameters.var(name));
            }
        }

        #endregion

        #region Private methods

        private void SetEnvVar(string name, string value)
        {
            if (!this.isShellMode)
            {
                return;
            }
            Environment.SetEnvironmentVariable(this.schedulerVariableNamePrefix + name.ToUpper(), value);
        }

        #endregion
    }
}
