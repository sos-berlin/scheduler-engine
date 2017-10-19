namespace sos.spooler
{
    using System;
    using System.Globalization;
    using System.Management.Automation.Host;
    using System.Text.RegularExpressions;
    using System.Threading;

    internal class PowershellAdapterPSHost : PSHost
    {
        #region Constants and Fields

        private readonly CultureInfo currentCulture;
        private readonly CultureInfo currentUiCulture;
        private readonly Guid hostId;
        private readonly PowershellAdapterPSHostUserInterface ui;
        private readonly Task spooler_task;
        private int exitCode;

        #endregion

        #region Constructors and Destructors

        public PowershellAdapterPSHost(Task task, Log log)
        {
            hostId = Guid.NewGuid();
            ui = new PowershellAdapterPSHostUserInterface(log);
            spooler_task = task;
            var culture = Thread.CurrentThread.CurrentCulture;
            var uiCulture = Thread.CurrentThread.CurrentUICulture;
            if (culture != null && culture.Name != null)
            {
                if (Regex.IsMatch(culture.Name, "^ja-|^zh-|^ko-|^ar-", RegexOptions.IgnoreCase))
                {
                    culture = CultureInfo.InvariantCulture;
                    uiCulture = CultureInfo.InvariantCulture;
                    Thread.CurrentThread.CurrentCulture = culture;
                    Thread.CurrentThread.CurrentUICulture = uiCulture;
                }
            }
            currentCulture = culture;
            currentUiCulture = uiCulture;
        }

        #endregion

        #region Public Properties

        #region Properties
        //powershell exit
        public int ExitCode
        {
            get
            {
                return exitCode;
            }
        }

        //windows native program exit
        public int LastExitCode { get; set; }

        public String LastFunctionWithExitCode { get; private set; }

        #endregion

        #region Override

        public override CultureInfo CurrentCulture
        {
            get
            {
                return currentCulture;
            }
        }

        public override CultureInfo CurrentUICulture
        {
            get
            {
                return currentUiCulture;
            }
        }

        public override Guid InstanceId
        {
            get
            {
                return hostId;
            }
        }

        public override string Name
        {
            get
            {
                return "JobSchedulerPowershellAdapterPSHost";
            }
        }

        public override PSHostUserInterface UI
        {
            get
            {
                return ui;
            }
        }

        public override Version Version
        {
            get
            {
                return new Version(1, 0, 0, 0);
            }
        }

        #endregion

        #endregion

        #region Public Methods

        public override void EnterNestedPrompt()
        {
            throw new NotImplementedException();
        }

        public override void ExitNestedPrompt()
        {
            throw new NotImplementedException();
        }

        public override void NotifyBeginApplication()
        {
        }

        public override void NotifyEndApplication()
        {
        }

        public override void SetShouldExit(int shouldExitCode)
        {
            if (exitCode == 0)
            {
                if (shouldExitCode != 0)
                {
                    exitCode = shouldExitCode;
                    LastFunctionWithExitCode = ui.CurrentFunctionName;
                    ui.WriteExitCodeError(exitCode);
                    spooler_task.set_exit_code(exitCode);
                }
            }
            else if (shouldExitCode != LastExitCode)
            {
                LastFunctionWithExitCode = ui.CurrentFunctionName;
                ui.WriteExitCodeWarning(shouldExitCode);
            }
        }

        #endregion        
    }
}
