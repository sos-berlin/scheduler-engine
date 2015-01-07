package com.sos.scheduler.engine.taskserver.task.process

import com.sos.scheduler.engine.common.scalautil.FileUtils.implicits._
import com.sos.scheduler.engine.common.system.OperatingSystem._
import java.nio.charset.Charset
import java.nio.charset.StandardCharsets._
import java.nio.file.Files.createTempFile
import java.nio.file.attribute.PosixFilePermissions
import java.nio.file.attribute.PosixFilePermissions._
import java.nio.file.{Files, Path}
import scala.collection.immutable
import scala.util.control.NonFatal

/**
 * @author Joacim Zschimmer
 */
object ShellProcessStarter {

  def start(name: String, scriptString: String): ShellProcess = {
    val file = OS.newTemporaryFile(name = name)
    try {
      file.toFile.write(scriptString, OS.fileEncoding)
      val processBuilder = new ProcessBuilder
      processBuilder.command(OS.toCommandArguments(file): _*)
      val process = processBuilder.start()
      process.getOutputStream.close() // Empty stdin
      val shellProcess = new ShellProcess(process)
      shellProcess.onClose { Files.delete(file) }
      shellProcess
    }
    catch { case NonFatal(t) ⇒ file.delete(); throw t }
  }

  private val OS = if (isWindows) ShellProcessStarterWindowsSpecific else ShellProcessStarterUnixSpecific

  private trait OperatingSystemSpecific {
    val fileEncoding: Charset
    def newTemporaryFile(name: String): Path
    def toCommandArguments(file: Path): immutable.Seq[String]
    protected final def filenamePrefix(name: String) = s"JobScheduler-Agent-$name-"
  }

  private object ShellProcessStarterUnixSpecific extends ShellProcessStarter.OperatingSystemSpecific {
    private val fileAttribute = asFileAttribute(PosixFilePermissions fromString "rwx------")
    val fileEncoding = UTF_8  // Umgebungsvariable beachten??? LANG=de_DE.UTF_8
    def newTemporaryFile(name: String) = createTempFile(filenamePrefix(name), ".sh", fileAttribute)
    def toCommandArguments(file: Path) = Vector("/bin/sh", file.toString)
  }

  private object ShellProcessStarterWindowsSpecific extends ShellProcessStarter.OperatingSystemSpecific {
    val fileEncoding = ISO_8859_1
    def newTemporaryFile(name: String) = createTempFile(filenamePrefix(name), ".cmd")
    def toCommandArguments(file: Path) = Vector("""C:\Windows\System32\cmd.exe""", "/C", file.toString)
  }
}
