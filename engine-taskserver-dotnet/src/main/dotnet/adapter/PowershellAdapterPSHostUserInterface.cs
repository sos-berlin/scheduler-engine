namespace sos.spooler
{
    using System;
    using System.Collections.Generic;
    using System.Collections.ObjectModel;
    using System.Management.Automation;
    using System.Management.Automation.Host;
    using System.Security;

    internal class PowershellAdapterPSHostUserInterface : PSHostUserInterface
    {
        #region Constants and Fields

        private readonly PowershellAdapterPSHostRawUserInterface rawUi;
        private readonly Log spooler_log;
        private string currentFunctionName;
        private string currentFunctionNameFormatted;

        #endregion

        #region Constructors and Destructors

        public PowershellAdapterPSHostUserInterface(Log log)
        {
            spooler_log = log;
            rawUi = new PowershellAdapterPSHostRawUserInterface();
        }

        #endregion

        #region Public Properties

        public String CurrentFunctionName
        {
            get
            {
                return currentFunctionName;
            }
            set
            {
                currentFunctionName = value;
                currentFunctionNameFormatted = String.IsNullOrEmpty(currentFunctionName) ? "" : String.Format("[{0}] ", currentFunctionName);
            }
        }

        public String LastInfoMessage { get; set; }

        public override PSHostRawUserInterface RawUI
        {
            get
            {
                return rawUi;
            }
        }

        #endregion

        #region Public Methods

        #region Methods
        public void WriteExitCodeError(int exitCode)
        {
            spooler_log.error(String.Format(
                    "{0}Process terminated with exit code {1}. See the following warning SCHEDULER-280.", currentFunctionNameFormatted, exitCode));
        }

        public void WriteExitCodeWarning(int exitCode)
        {
            spooler_log.warn(String.Format(
                    "{0}Process terminated with exit code {1}.", currentFunctionNameFormatted, exitCode));
        }

        #endregion

        #region Override Methods 
        public override Dictionary<string, PSObject> Prompt(
            string caption, string message, Collection<FieldDescription> descriptions)
        {
            throw new NotImplementedException();
        }

        public override int PromptForChoice(
            string caption, string message, Collection<ChoiceDescription> choices, int defaultChoice)
        {
            throw new NotImplementedException();
        }

        public override PSCredential PromptForCredential(
            string caption, string message, string userName, string targetName)
        {
            throw new NotImplementedException();
        }

        public override PSCredential PromptForCredential(
            string caption, string message, string userName, string targetName, PSCredentialTypes allowedCredentialTypes,
            PSCredentialUIOptions options)
        {
            throw new NotImplementedException();
        }

        public override string ReadLine()
        {
            throw new NotImplementedException();
        }

        public override SecureString ReadLineAsSecureString()
        {
            throw new NotImplementedException();
        }

        public override void Write(string value)
        {
            WriteInfo(value);
        }

        public override void Write(ConsoleColor foregroundColor, ConsoleColor backgroundColor, string value)
        {
            WriteInfo(value);
        }

        public override void WriteDebugLine(string message)
        {
            spooler_log.debug3(GetOutputMessage(message));
        }

        public override void WriteErrorLine(string message)
        {
            Console.Error.WriteLine(String.Format("{0}{1}", currentFunctionNameFormatted, GetOutputMessage(message)));
        }

        public override void WriteLine(string value)
        {
            WriteInfo(value);
        }

        public override void WriteProgress(long sourceId, ProgressRecord record)
        {
        }

        public override void WriteVerboseLine(string message)
        {
            spooler_log.debug(GetOutputMessage(message));
        }

        public override void WriteWarningLine(string message)
        {
            spooler_log.warn(GetOutputMessage(message));
        }

        #endregion

        #endregion

        #region Methods

        private static string GetOutputMessage(string msg)
        {
            return msg ?? "";
        }

        private void WriteInfo(string message)
        {
            var val = GetOutputMessage(message);
            LastInfoMessage = val;
            spooler_log.info(val);
        }

        #endregion
    }
}
