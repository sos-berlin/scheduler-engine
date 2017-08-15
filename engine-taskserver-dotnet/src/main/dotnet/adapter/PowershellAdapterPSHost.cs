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

        #endregion

        #region Constructors and Destructors

        public PowershellAdapterPSHost(Log log)
        {
            this.hostId = Guid.NewGuid();
            this.ui = new PowershellAdapterPSHostUserInterface(log);
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
            this.currentCulture = culture;
            this.currentUiCulture = uiCulture;
        }

        #endregion

        #region Public Properties

        public override CultureInfo CurrentCulture
        {
            get
            {
                return this.currentCulture;
            }
        }

        public override CultureInfo CurrentUICulture
        {
            get
            {
                return this.currentUiCulture;
            }
        }

        public override Guid InstanceId
        {
            get
            {
                return this.hostId;
            }
        }

        public override string Name
        {
            get
            {
                return "PowershellAdapterPSHost";
            }
        }

        public override PSHostUserInterface UI
        {
            get
            {
                return this.ui;
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

        public override void SetShouldExit(int exitCode)
        {
        }

        #endregion
    }
}
