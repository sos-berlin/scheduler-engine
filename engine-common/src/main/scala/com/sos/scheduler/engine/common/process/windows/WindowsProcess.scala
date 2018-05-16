package com.sos.scheduler.engine.common.process.windows

import com.sos.scheduler.engine.common.process.Processes.Pid
import com.sos.scheduler.engine.common.process.windows.CommandLineConversion.argsToCommandLine
import com.sos.scheduler.engine.common.process.windows.WindowsApi.{advapi32, call, handleCall, kernel32, myUserenv, openProcessToken, waitForSingleObject, windowsDirectory}
import com.sos.scheduler.engine.common.process.windows.WindowsProcess._
import com.sos.scheduler.engine.common.scalautil.AutoClosing.autoClosing
import com.sos.scheduler.engine.common.scalautil.FileUtils.implicits._
import com.sos.scheduler.engine.common.scalautil.{Logger, SetOnce}
import com.sun.jna.platform.win32.Advapi32Util.getEnvironmentBlock
import com.sun.jna.platform.win32.Kernel32Util.closeHandle
import com.sun.jna.platform.win32.WinBase._
import com.sun.jna.platform.win32.WinError.ERROR_ACCESS_DENIED
import com.sun.jna.platform.win32.WinNT._
import com.sun.jna.platform.win32.Wincon.{STD_ERROR_HANDLE, STD_INPUT_HANDLE, STD_OUTPUT_HANDLE}
import com.sun.jna.ptr.IntByReference
import com.sun.jna.{Structure, WString}
import java.io.OutputStream
import java.lang.Math.{max, min}
import java.lang.ProcessBuilder.Redirect
import java.lang.ProcessBuilder.Redirect.Type.{INHERIT, PIPE, WRITE}
import java.nio.charset.Charset
import java.nio.file.Path
import java.util.concurrent.TimeUnit
import java.util.concurrent.atomic.AtomicBoolean
import org.jetbrains.annotations.TestOnly
import scala.collection.JavaConversions._
import scala.collection.mutable
import scala.io.Codec
import scala.util.Try
import scala.util.control.NonFatal

/**
  * A Windows process, started with CreateProcessW via JNA.
  *
  * @author Joacim Zschimmer
  */
final class WindowsProcess private(processInformation: PROCESS_INFORMATION,
  inRedirection: Redirection, outRedirection: Redirection, errRedirection: Redirection,
  loggedOn: LoggedOn)
extends Process with AutoCloseable {

  val pid = Pid(processInformation.dwProcessId.intValue)
  private val exitValueOnce = new SetOnce[Int]
  private val handleGuard = new ResourceGuard(processInformation.hProcess) {
    def release(hProcess: HANDLE) = {
      closeHandle(hProcess)
      inRedirection.closePipe()
      outRedirection.closePipe()
      errRedirection.closePipe()
      loggedOn.close()
    }
  }

  /** stdin. */
  lazy val getOutputStream: OutputStream = {
    if (inRedirection.pipeHandle == INVALID_HANDLE_VALUE) throw new IllegalStateException("WindowsProcess has no handle for stdin attached")
    new PipeOutputStream(inRedirection.pipeHandle) {
      override def close() = inRedirection.closePipe()
    }
  }

  def getInputStream = throw new UnsupportedOperationException("WindowsProcess provides no stdout InputStream")

  def getErrorStream = throw new UnsupportedOperationException("WindowsProcess provides no stderr InputStream")

  override def finalize() = {
    handleGuard.releaseAfterUse()
    super.finalize()
  }

  /**
    * Implicitly called by `waitFor` and `exitValue`, if process has terminated.
    * After this call, `exitValue` (`GetExitCodeProcess`) can no longer be read if not already done.
    */
  def close() = handleGuard.releaseAfterUse()

  override def waitFor(timeout: Long, unit: TimeUnit) =
    exitValueOnce.isDefined ||
      waitForProcess(max(0, min(Int.MaxValue, TimeUnit.MILLISECONDS.convert(timeout, unit))).toInt)

  def waitFor() =
    exitValueOnce getOrElse {
      waitForProcess(INFINITE)
      exitValueOnce()
    }

  override def toString = s"WindowsProcess($pid)"

  override def isAlive = exitValueOnce.isEmpty && !waitForProcess(0)

  def exitValue =
    exitValueOnce getOrElse {
      if (!waitForProcess(0)) throw new IllegalThreadStateException(s"Process $pid has not yet terminated")
      exitValueOnce()
    }

  private def waitForProcess(timeout: Int): Boolean =
    handleGuard {
      case Some(hProcess) ⇒
        val terminated = waitForSingleObject(hProcess, timeout)
        if (terminated) {
          exitValueOnce.trySet(getExitCodeProcess(hProcess))
          handleGuard.releaseAfterUse()
        }
        terminated
      case None ⇒
        throw new IllegalStateException("WindowsProcess has already been closed")
    }

  def destroy() =
    handleGuard {
      case Some(hProcess) ⇒
        call("TerminateProcess") {
          kernel32.TerminateProcess(hProcess, TerminateProcessExitValue) ||
            kernel32.GetLastError == ERROR_ACCESS_DENIED && {
              (Try { waitForProcess(0) } getOrElse false) || {
                kernel32.SetLastError(ERROR_ACCESS_DENIED)
                false
              }
            }
        }
      case None ⇒
    }
}

object WindowsProcess {
  private[windows] val TerminateProcessExitValue = 999999999
  val WindowsProcessTargetSystemProperty = "jobscheduler.WindowsProcess.target"
  val AuthenticatedUsersSid = "S-1-5-11"
  private val InheritableEnvironmentVariables = Vector(  // JS-1747
    "SCHEDULER_DATA",
    "SCHEDULER_HOME",
    "SCHEDULER_HTTP_PORT",
    "SCHEDULER_HTTPS_PORT",
    "SCHEDULER_LOG_DIR",
    "SCHEDULER_LOGFILE",
    "SCHEDULER_PID",
    "SCHEDULER_PID_FILE",
    "SCHEDULER_PORT",
    "SCHEDULER_WORK_DIR")
  private val logger = Logger(getClass)

  object environment {
    private lazy val _env = mutable.Map[String, String]() ++ sys.env

    def env = _env.toMap

    @TestOnly
    def set(key: String, value: String): Unit =
      _env(key) = value

    @TestOnly
    def delete(key: String): Unit =
      _env -= key
  }

  def start(processBuilder: ProcessBuilder, logon: Option[Logon] = None): WindowsProcess = {
    val loggedOn = LoggedOn.logon(logon)
    val inRedirection = redirectToHandle(STD_INPUT_HANDLE, processBuilder.redirectInput)
    val outRedirection = redirectToHandle(STD_OUTPUT_HANDLE, processBuilder.redirectOutput)
    val errRedirection = redirectToHandle(STD_ERROR_HANDLE, processBuilder.redirectError)
    val startupInfo = new STARTUPINFO
    startupInfo.dwFlags |= STARTF_USESTDHANDLES
    startupInfo.hStdInput = inRedirection.startupInfoHandle
    startupInfo.hStdOutput = outRedirection.startupInfoHandle
    startupInfo.hStdError = errRedirection.startupInfoHandle

    val application = processBuilder.command().head
    val commandLine = argsToCommandLine(processBuilder.command.toIndexedSeq)
    val creationFlags = CREATE_UNICODE_ENVIRONMENT
    val env = logon match {
      case Some(o) if o.withUserProfile ⇒
        inheritedUsersEnvironment(loggedOn.userToken)  // Only reliable if user profile has been loaded (see JS-1725)
      case Some(o) ⇒
        inheritedUsersEnvironment(null) ++  // Default system environment
          Some("USERNAME" → o.user.withoutDomain) ++ // Default system environment contains default USERNAME and USERDOMAIN. We change this..
          (o.user.domain orElse sys.env.get("USERDOMAIN") map "USERDOMAIN".→)
      case None ⇒
        sys.env
    }
    val directory = windowsDirectory.getRoot.toString  // Need a readable directory, ignoring ProcessBuilder#directory
    val processInformation = new PROCESS_INFORMATION
    call("CreateProcessAsUser", application, commandLine, s"directory=$directory") {
      advapi32.CreateProcessAsUser(loggedOn.userToken, application, commandLine,
        null: SECURITY_ATTRIBUTES, null: SECURITY_ATTRIBUTES, /*inheritHandles=*/true, creationFlags,
        getEnvironmentBlock(env ++ processBuilder.environment),
        directory, startupInfo, processInformation)
    }
    inRedirection.releaseStartupInfoHandle()
    outRedirection.releaseStartupInfoHandle()
    errRedirection.releaseStartupInfoHandle()
    closeHandle(processInformation.hThread)
    processInformation.hThread = INVALID_HANDLE_VALUE
    new WindowsProcess(processInformation, inRedirection, outRedirection, errRedirection, loggedOn)
  }

  private def inheritedUsersEnvironment(userToken: HANDLE) =
    WindowsApi.usersEnvironment(userToken) ++ {
      val env = environment.env
      InheritableEnvironmentVariables collect { case k if env isDefinedAt k ⇒ k → env(k) }
    }

  private class LoggedOn(val userToken: HANDLE, val profileHandle: HANDLE = INVALID_HANDLE_VALUE)
  extends AutoCloseable {
    private val closed = new AtomicBoolean

    def close() = {
      if (!closed.getAndSet(true)) {
        if (profileHandle != INVALID_HANDLE_VALUE) {
          call("UnloadUserProfile") {
            myUserenv.UnloadUserProfile(userToken, profileHandle)
          }
        }
        closeHandle(userToken)
      }
    }
  }

  private object LoggedOn {
    def logon(logonOption: Option[Logon]): LoggedOn =
      logonOption match {
        case Some(o) ⇒ logon(o)
        case None ⇒ new LoggedOn(openProcessToken(kernel32.GetCurrentProcess, TOKEN_ALL_ACCESS))
      }

    private def logon(logon: Logon): LoggedOn = {
      import logon.{credentials, user, withUserProfile}
      logger.debug(s"LogonUser '$user'")
      val userToken = handleCall("LogonUser")(
        advapi32.LogonUser(user.string, null, credentials.password.string, LOGON32_LOGON_BATCH, LOGON32_PROVIDER_DEFAULT, _))
      new LoggedOn(
        userToken,
        profileHandle =
          if (withUserProfile)
            loadUserProfile(userToken, user)
          else
            INVALID_HANDLE_VALUE)
    }
  }

  private def loadUserProfile(userToken: HANDLE, user: WindowsUserName): HANDLE = {
    val profileInfo = Structure.newInstance(classOf[MyUserenv.PROFILEINFO]).asInstanceOf[MyUserenv.PROFILEINFO]
    profileInfo.dwSize = profileInfo.size
    profileInfo.userName = new WString(user.string)
    profileInfo.write()
    call("LoadUserProfile") {
      myUserenv.LoadUserProfile(userToken, profileInfo)
    }
    profileInfo.read()
    profileInfo.hProfile
  }

  private def redirectToHandle(stdFile: Int, redirect: Redirect): Redirection =
    redirect.`type` match {
      case INHERIT ⇒
        new Redirection(kernel32.GetStdHandle(stdFile), false, INVALID_HANDLE_VALUE)

      case PIPE ⇒
        stdFile match {
          case STD_INPUT_HANDLE ⇒
            Redirection.forStdinPipe

          case STD_OUTPUT_HANDLE | STD_ERROR_HANDLE ⇒
            new Redirection(new HANDLE, false, INVALID_HANDLE_VALUE)  // We do not support the default PIPE, so we return empty values
        }

      case WRITE ⇒
        Redirection.forDirectFile(redirect.file)

      case t ⇒ throw new IllegalArgumentException(s"Unsupported Redirect $t")
  }

  private def getExitCodeProcess(hProcess: HANDLE): Int = {
    val ref = new IntByReference
    call("GetExitCodeProcess") {
      kernel32.GetExitCodeProcess(hProcess, ref)
    }
    ref.getValue
  }

  /**
    * Adds the needed ACL despite any existing ACL.
    */
  def makeFileExecutableForUser(file: Path, user: WindowsUserName): file.type =
    grantFileAccess(file, s"${injectableUserName(user)}:RX")

  /**
    * Adds the needed ACL despite any existing ACL.
    */
  def makeFileExecutableForEverybody(file: Path): file.type =
    grantFileAccess(file, s"*$AuthenticatedUsersSid:RX")

  /**
    * Adds the needed ACL despite any existing ACL.
    */
  def makeFileAppendableForUser(file: Path, user: WindowsUserName): file.type = {
    grantFileAccess(file, s"${injectableUserName(user)}:M")
  }

  private val AllowedUserNameCharacters = Set('_', '.', '-', ',', ' ', '@')  // Only for icacls syntactically irrelevant characters and domain separators

  private[windows] def injectableUserName(user: WindowsUserName): String = {
    val name = user.string
    def isValid(c: Char) = c.isLetterOrDigit || AllowedUserNameCharacters(c)
    require(name.nonEmpty && name.forall(isValid), s"Unsupported character in Windows user name: '$name'")  // Avoid code injection
    name
  }

  def makeDirectoryAccessibleForEverybody(directory: Path): directory.type =
    grantFileAccess(directory, s"*$AuthenticatedUsersSid:(OI)RX")

  /**
    * @param grant syntax is weakly checked!
    */
  private def grantFileAccess(file: Path, grant: String): file.type = {
    execute(windowsDirectory / "System32\\icacls.exe", '"' + file.toString + '"', "/q", "/grant", grant)
    file
  }

  def execute(executable: Path, args: String*): Vector[String] = {
    logger.debug(executable + args.mkString(" [", ", ", "]"))
    val process = new ProcessBuilder(executable.toString +: args: _*).redirectErrorStream(true).start()
    process.getOutputStream.close()  // stdin
    val lines = {
      val commandCodec = new Codec(Charset forName "cp850")
      try autoClosing(process.getInputStream) { in ⇒ io.Source.fromInputStream(in)(commandCodec).getLines().toVector }
      catch { case NonFatal(t) ⇒
        Vector(s"error message not readable: $t")
      }
    }
    val returnCode = process.waitFor()
    if (returnCode != 0) throw new RuntimeException(s"Windows command failed: $executable => ${lines mkString " / "}")
    lines
  }
}
