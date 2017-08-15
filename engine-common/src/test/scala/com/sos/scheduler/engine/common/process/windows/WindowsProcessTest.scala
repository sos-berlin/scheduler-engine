package com.sos.scheduler.engine.common.process.windows

import com.sos.scheduler.engine.common.process.windows.WindowsApi.windowsDirectory
import com.sos.scheduler.engine.common.process.windows.WindowsProcess.WindowsProcessTargetSystemProperty
import com.sos.scheduler.engine.common.scalautil.FileUtils.implicits._
import com.sos.scheduler.engine.common.system.OperatingSystem.isWindows
import java.lang.ProcessBuilder.Redirect
import java.lang.ProcessBuilder.Redirect.INHERIT
import java.nio.charset.Charset
import java.nio.charset.StandardCharsets.US_ASCII
import java.nio.file.Files.{createTempFile, delete}
import java.nio.file.{Files, Path}
import java.util.Locale
import java.util.concurrent.TimeUnit.{MILLISECONDS, SECONDS}
import org.junit.runner.RunWith
import org.scalatest.FreeSpec
import org.scalatest.Matchers._
import org.scalatest.junit.JUnitRunner
import scala.collection.JavaConversions._
import scala.util.{Failure, Try}

/**
  * @author Joacim Zschimmer
  */
@RunWith(classOf[JUnitRunner])
final class WindowsProcessTest extends FreeSpec {

  if (isWindows) {
    val commandCharset = Charset forName "cp850"
    lazy val logon = sys.props.get(WindowsProcessTargetSystemProperty)
      .filter { _.nonEmpty }
      .map { o ⇒ Logon(WindowsProcessCredentials.byKey(o), withUserProfile = false) }

    "CreateProcess" in {
      val batchFile = /*credentials match {
        case Some(WindowsProcessCredentials(user, _)) ⇒
          val directory = createTempFile(WindowsApi.windowsDirectory / "Temp", "WindowsProcessTest-", ".cmd")
          WindowsProcess.makeFileExecutableForUser(directory, user)
        case None ⇒*/
          createTempFile("WindowsProcessTest-", ".cmd")
      //}
      batchFile.contentString =
       s"""rem @echo off
          |set
          |dir
          |echo TEST-USERNAME=SELF-TEST
          |echo TEST-STDERR 1>&2
          |echo ENV-USERNAME=%USERNAME%
          |for /f "usebackq tokens=*" %%a in (`echo SELF-TEST`) do echo TEST-USERNAME=%%a
          |for /f "usebackq tokens=*" %%a in (`$windowsDirectory\\system32\\whoami.exe`) do echo TEST-USERNAME=%%a
          |rem $windowsDirectory\\system32\\whoami.exe >$windowsDirectory\\Temp\\WindowsProcessTest.log
          |rem Cygwin bricht das Skript ab: whoami.exe
          |ping -n 3 %1""".stripMargin
      val stdoutFile = createTempFile("test-stdout-", ".log")
      val stderrFile = createTempFile("test-stderr-", ".log")
      val processBuilder = new ProcessBuilder(batchFile.toString, "127.0.0.1")
        .redirectInput(Redirect.INHERIT)
        .redirectOutput(Redirect to stdoutFile)
        .redirectError(Redirect to stderrFile)
      processBuilder.environment.clear()
      val process = WindowsProcess.start(processBuilder, logon)
      assert(process.isAlive)
      intercept[Exception] {
        process.exitValue
      }
      process.waitFor(10, MILLISECONDS) shouldBe false
      val processResult = process.waitFor(5, SECONDS)
      assert(processResult)
      assert(!process.isAlive)
      assert(process.exitValue == 0)
      assert(Files.size(stdoutFile) > 0)
      val stdout = stdoutFile.contentString(commandCharset)
      println(stdout)
      assert(stdout contains "TEST-STDERR")
      assert(stdout contains "127.0.0.1")
      val userNameLines = stdout split "\n" filter { _ contains "TEST-USERNAME=" } map { _.trim.toLowerCase(Locale.ROOT) }
      assert(userNameLines exists { _ endsWith "self-test" })
      val me = sys.env("USERNAME").toLowerCase(Locale.ROOT)
      logon match {
        case Some(Logon(WindowsProcessCredentials(WindowsUserName(user), _), _)) ⇒
          assert(stdout.toLowerCase(Locale.ROOT) contains s"ENV-USERNAME=$user".toLowerCase(Locale.ROOT))
          assert(userNameLines forall { o ⇒ !o.endsWith("\\" ++ me) })
          // whoami outputs nothing, but quits whole command ???
          //assert(userNameLines forall { _ endsWith ("\\" + user) })  // whoami outputs domain backslash username
        case None ⇒
          // "USERNAME=Lokaler Dienst"? assert(stdout contains s"ENV-USERNAME=$me")
          assert(userNameLines exists { _.endsWith("\\" ++ me) })
      }
      delete(stdoutFile)
      delete(stderrFile)
      delete(batchFile)
    }

    "TerminateProcess" in {
      val processBuilder = new ProcessBuilder(s"$windowsDirectory\\system32\\ping.exe", "-n", "1", "127.0.0.1")
      val process = WindowsProcess.start(processBuilder, logon)
      process.destroy()
      process.waitFor(1, SECONDS)
      assert(!process.isAlive)
      assert(process.exitValue == WindowsProcess.TerminateProcessExitValue)
    }

    "Inherit stdout (manual test)" in {
      val processBuilder = new ProcessBuilder(s"$windowsDirectory\\system32\\ping.exe", "-n", "1", "127.0.0.1")  // Please check stdout yourself
        .redirectOutput(INHERIT)
      val process = WindowsProcess.start(processBuilder, logon)
      process.waitFor(5, SECONDS) shouldBe true
      assert(process.exitValue == 0)
    }

    "Environment" in {
      val testVariableName = "TEST_VARIABLE"
      val testVariableValue = "TEST-VALUE"
      val scriptFile = createTempFile("test-", ".cmd")
      scriptFile.contentString = s"""
        |@echo off
        |echo $testVariableName=%$testVariableName%
        |exit 0
        |""".stripMargin
      val stdoutFile = createTempFile("test-", ".log")
      val processBuilder = new ProcessBuilder(s"$windowsDirectory\\system32\\cmd.exe", "/C", scriptFile.toString)
        .redirectOutput(Redirect to stdoutFile)
      processBuilder.environment ++= Map(testVariableName → testVariableValue)
      val process = WindowsProcess.start(processBuilder, logon)
      process.waitFor(5, SECONDS) shouldBe true
      assert(process.exitValue == 0)
      assert(stdoutFile.contentString(commandCharset) contains s"$testVariableName=$testVariableValue")
      delete(stdoutFile)
    }

    "Write to stdin" in {
      val scriptFile = createTempFile("test-", ".cmd")
      scriptFile.contentString = """
        |@echo off
        |set /p input=
        |echo input=%input%
        |""".stripMargin
      val stdoutFile = createTempFile("test-", ".log")
      val processBuilder = new ProcessBuilder(scriptFile.toString)
        .redirectOutput(Redirect to stdoutFile)
      val testString = "HELLO, THIS IS A TEST\r\n"
      val testBytes = testString.getBytes(US_ASCII)
      val process = WindowsProcess.start(processBuilder, logon)
      process.getOutputStream.write(testBytes, 0, testBytes.length)
      process.getOutputStream.flush()
      process.waitFor(5, SECONDS) shouldBe true
      stdoutFile.contentString(commandCharset) shouldEqual s"input=$testString"
      delete(stdoutFile)
      delete(scriptFile)
    }

    for (user ← logon map { _.user }) {
      "makeFileExecutableForUser" in {
        check("(RX)")(WindowsProcess.makeFileExecutableForUser(_, user))
      }

      "makeFileAppendableForUser" in {
        check("(M)")(WindowsProcess.makeFileAppendableForUser(_, user))
      }

      def check(expected: String)(body: Path ⇒ Unit): Unit = {
        val file = createTempFile("test-", ".tmp")
        body(file)
        val icaclsOut = icacls(file)
        println(icaclsOut)
        assert(icaclsOut.replace("\r\n", "\n").toLowerCase(Locale.ROOT) contains s"$user:$expected\n".toLowerCase(Locale.ROOT))
        delete(file)
      }

      "makeFileAppendableForUser, script appends" in {
        val appendableFile = createTempFile("test-", ".tmp")
        appendableFile.contentString = ""
        WindowsProcess.makeFileAppendableForUser(appendableFile, user)
        val stdoutFile = createTempFile("test-", ".tmp")
        println(icacls(appendableFile))
        val scriptFile = createTempFile("test-", ".cmd")
        scriptFile.contentString = s"echo TEST>>$appendableFile\n"
        WindowsProcess.start(new ProcessBuilder(scriptFile.toString).redirectOutput(Redirect to stdoutFile), logon).waitFor()
        println(stdoutFile.contentString(commandCharset))
        delete(stdoutFile)
        delete(scriptFile)
        assert(appendableFile.contentString == "TEST\r\n")
        delete(appendableFile)
      }
    }

    if (false) {  // Own user: stdout is empty. Other user: DLL initialization error, user32.dll
      "Java via script" in {
        val scriptFile = createTempFile("test-", ".cmd")
        scriptFile.contentString = """
          |@echo off
          |java -version
          |""".stripMargin
        val stdoutFile = createTempFile("test-", ".log")
        val processBuilder = new ProcessBuilder(scriptFile.toString)
          .redirectOutput(Redirect to stdoutFile)
        val process = WindowsProcess.start(processBuilder, logon)
        process.waitFor()
        println(f"exitValue=${process.exitValue}%08x")
        assert(process.exitValue == 0)
        assert(stdoutFile.contentString(commandCharset) contains "java version 1.8.0_131")
        delete(stdoutFile)
        delete(scriptFile)
      }

      "Java direct" in {
        val stdoutFile = createTempFile("test-", ".log")
        val stderrFile = createTempFile("test-", ".log")
        val processBuilder = new ProcessBuilder("""C:\Program Files\Java\jdk1.8.0_131\bin\java.exe""", "-version")
          .redirectOutput(Redirect to stdoutFile)
          .redirectOutput(Redirect to stderrFile)
        val process = WindowsProcess.start(processBuilder, logon map { _.copy(withUserProfile = true) })
        process.waitFor()
        println(stdoutFile.contentString(commandCharset))
        println(stderrFile.contentString(commandCharset))
        println(f"exitValue=${process.exitValue}%08x")
        assert(process.exitValue == 0)
        assert(stdoutFile.contentString(commandCharset) contains "java version 1.8.0_131")
        delete(stdoutFile)
        delete(stderrFile)
      }
    }

    def icacls(file: Path): String = {
      val stdoutFile = createTempFile("test-", ".log")
      new ProcessBuilder(s"$windowsDirectory\\system32\\icacls.exe", file.toString)
        .redirectOutput(Redirect to stdoutFile)
        .start()
        .waitFor()
      try stdoutFile.contentString(commandCharset)
      finally delete(stdoutFile)
    }

    "execute" in {
      val file = "NON-EXISTANT-FILE-WITH-ÜMLÅÙTS"
      val user = "NON-EXISTENT-ÜSËR"
      val Failure(t) = Try {
        WindowsProcess.execute(windowsDirectory / "System32\\icacls.exe", file, "/q", "/grant", s"$user:R")
      }
      val Array(a, b) = t.getMessage split "=>"
      println(t.getMessage)
      assert(a contains "icacls.exe")
      assert(b contains user)
    }
  }

  "injectableUserName" in {
    check("a")
    check("å")
    check("a.b")
    check(".a_b")
    check("_a.b")
    check("-a _ . , b")
    check("a b")
    check("user@domain")
    check("漢字")
    check("片仮名")
    def check(name: String) = assert(WindowsProcess.injectableUserName(WindowsUserName(name)) == name)
    intercept[IllegalArgumentException] { WindowsProcess.injectableUserName(WindowsUserName("")) }
    intercept[IllegalArgumentException] { WindowsProcess.injectableUserName(WindowsUserName("a%")) }
  }
}
