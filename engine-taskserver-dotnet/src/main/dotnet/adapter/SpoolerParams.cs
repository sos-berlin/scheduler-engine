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

        public SpoolerParams(Task task, Spooler sp, bool orderJob, bool shellMode)
        {
            spoolerTask = task;
            spooler = sp;
            isOrderJob = orderJob;
            isShellMode = shellMode;
            if (isShellMode)
            {
                schedulerVariableNamePrefix = spooler.variables().value("scheduler.variable_name_prefix");
            }
        }

        #endregion

        #region Public Methods

        public Variable_set getAll()
        {
            var parameters = spooler.create_variable_set();
            parameters.merge(spoolerTask.@params());
            if (isOrderJob)
            {
                var op = spoolerTask.order();
                if (op != null)
                {
                    parameters.merge(op.@params());
                }
            }
            return parameters;
        }

        public string get(string name)
        {
            var result = "";
            if (isOrderJob)
            {
                var op = spoolerTask.order();
                if (op != null)
                {
                    result = op.@params().var(name);
                }
            }
            return string.IsNullOrEmpty(result) ? spoolerTask.@params().var(name) : result;
        }

        public string value(string name)
        {
            return get(name);
        }

        public void set(string name, string value)
        {
            if (isOrderJob)
            {
                var op = spoolerTask.order();
                if (op != null)
                {
                    op.@params().set_var(name, value);
                }
            }
            else
            {
                spoolerTask.@params().set_var(name, value);
            }

            if (isShellMode)
            {
                SetEnvVar(name, value);
            }
        }

        #endregion

        #region Internal methods

        internal void SetEnvVars()
        {
            if (!isShellMode)
            {
                return;
            }

            var parameters = getAll();
            var parameterNames = getAll().names();
            if (!String.IsNullOrEmpty(parameterNames))
            {
                var names = parameterNames.Split(';');
                foreach (var name in names)
                {
                    SetEnvVar(name, parameters.var(name));
                }
            }
        }

        #endregion

        #region Private methods

        private void SetEnvVar(string name, string value)
        {
            Environment.SetEnvironmentVariable(schedulerVariableNamePrefix + name.ToUpper(), value);
        }

        #endregion
    }
}
