namespace sos.spooler
{
    using System;
    using System.Collections.Generic;
    using System.Dynamic;

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

            this.schedulerVariableNamePrefix = this.spooler.variables().value("scheduler.variable_name_prefix");
        }

        #endregion

        #region Public Methods

        public Variable_set get()
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

        public dynamic items
        {
            get
            {
                dynamic eo = new ExpandoObject();
                var d = eo as IDictionary<String, object>;
                var parameters = this.get();
                var names = parameters.names().Split(';');
                foreach (var name in names)
                {
                    d[name] = parameters.var(name);
                }
                return eo;
            }
        }

        #endregion

        #region Internal methods

        internal void SetEnvVars()
        {
            var parameters = this.get();
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
            Environment.SetEnvironmentVariable(this.schedulerVariableNamePrefix + name.ToUpper(), value);
        }

        #endregion
    }
}
