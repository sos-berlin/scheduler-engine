namespace sos.spooler
{
    using System;
    using System.Collections.Generic;
    using System.Collections.ObjectModel;
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
            var results = this.InvokeCommand("spooler_task_before");
            var index = GetReturnValueIndex(results);
            this.Log(results, index);
            return GetReturnValue(results, index, true);
        }

        public override void spooler_task_after()
        {
            var results = this.InvokeCommand("spooler_task_after");
            this.Log(results);
        }

        public override bool spooler_process_before()
        {
            var results = this.InvokeCommand("spooler_process_before");
            var index = GetReturnValueIndex(results);
            this.Log(results, index);
            return GetReturnValue(results, index, true);
        }

        public override bool spooler_process_after(bool spoolerProcessResult)
        {
            var results = this.InvokeCommand("spooler_process_after",spoolerProcessResult);
            var index = GetReturnValueIndex(results);
            this.Log(results, index);
            return GetReturnValue(results, index, true);
        }

        public override bool spooler_init()
        {
            var results = this.InvokeCommand("spooler_init");
            var index = GetReturnValueIndex(results);
            this.Log(results, index);
            return GetReturnValue(results,index, true);
        }
        
        public override bool spooler_open()
        {
            var results = this.InvokeCommand("spooler_open");
            var index = GetReturnValueIndex(results);
            this.Log(results, index);
            return GetReturnValue(results, index, true);
        }

        public override void spooler_close()
        {
            var results = this.InvokeCommand("spooler_close");
            this.Log(results);
        }

        public override void spooler_on_success()
        {
            var results = this.InvokeCommand("spooler_on_success");
            this.Log(results);
        }

        public override void spooler_on_error()
        {
            var results = this.InvokeCommand("spooler_on_error");
            this.Log(results);
        }

        public override bool spooler_process()
        {
            var results = this.InvokeCommand("spooler_process");
            var index = GetReturnValueIndex(results);
            this.Log(results, index);

            return GetReturnValue(results, index, this.spooler_task.order() != null);
        }

        public override void spooler_exit()
        {
            var results = this.InvokeCommand("spooler_exit");
            this.Log(results);
        }

        public override void Close()
        {
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

        private void Log(IEnumerable<PSObject> results,int returnValueIndex = -1)
        {
            this.LogResults(results, returnValueIndex);
            this.LogStreams();
        }

        private void LogResults(IEnumerable<PSObject> results, int returnValueIndex)
        {
            if (results == null)
            {
                return;
            }
            var i = 0;
            foreach (var psObject in results)
            {
                if (i != returnValueIndex)
                {
                    this.spooler_log.info(psObject.ToString());
                }
                i++;
            }
        }

        private void LogStreams()
        {
            if (this.Shell.Streams.Error.Count > 0)
            {
                this.spooler_log.error(this.Shell.Streams.Error[0].ToString());
            }
            if (this.Shell.Streams.Warning.Count > 0)
            {
                this.spooler_log.warn(this.Shell.Streams.Warning[0].ToString());
            }
            if (this.Shell.Streams.Debug.Count > 0)
            {
                this.spooler_log.debug1(this.Shell.Streams.Debug[0].ToString());
            }
            if (this.Shell.Streams.Verbose.Count > 0)
            {
                this.spooler_log.info(this.Shell.Streams.Verbose[0].ToString());
            }
            this.Shell.Streams.ClearStreams();
        }

        private Collection<PSObject> InvokeCommand(String methodName, bool? param = null)
        {
            this.Shell.Commands.Clear();
            var methodParams = "";
            if (param.HasValue)
            {
                var str = string.Concat("$", param.Value.ToString(CultureInfo.InvariantCulture));
                methodParams = "(" + str + ")";
            }
            this.Shell.AddScript("if ($function:" + methodName + ") {" + methodName + methodParams+ " | Out-String -Stream}");
            return this.Shell.Invoke();
        }

        private static int GetReturnValueIndex(IEnumerable<PSObject> results)
        {
            var index = -1;
            try
            {
                index = results.Select(
                    (v, i) => new
                    {
                        Value = v.ToString().ToLower(),
                        Index = i
                    }).Where(x => x.Value.Equals("true") || x.Value.Equals("false")).Select(x => x.Index).Last();

            }
            catch (Exception)
            {
            }
            return index;
        }

        private static bool GetReturnValue(IEnumerable<PSObject> results, int returnValueIndex, bool defaultValue)
        {
            var result = defaultValue;
            if (returnValueIndex > -1)
            {
                try
                {
                    result = Boolean.Parse(results.ElementAt(returnValueIndex).ToString());
                }
                catch (Exception ex)
                {
                }
            }
            return result;
        }

        #endregion
    }
}
