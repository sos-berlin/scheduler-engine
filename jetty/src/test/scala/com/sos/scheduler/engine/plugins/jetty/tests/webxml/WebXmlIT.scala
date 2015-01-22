package com.sos.scheduler.engine.plugins.jetty.tests.webxml

import com.sos.scheduler.engine.common.guice.GuiceImplicits._
import com.sos.scheduler.engine.common.scalautil.FileUtils.implicits._
import com.sos.scheduler.engine.common.system.OperatingSystem.isWindows
import com.sos.scheduler.engine.kernel.plugin.PluginSubsystem
import com.sos.scheduler.engine.plugins.jetty.JettyPlugin
import com.sos.scheduler.engine.plugins.jetty.test.{DirectoryListingTests, JettyPluginJerseyTester}
import com.sos.scheduler.engine.plugins.jetty.tests.webxml.WebXmlIT._
import com.sos.scheduler.engine.test.scalatest.ScalaSchedulerTest
import java.nio.file.Files
import java.nio.file.Files.{createDirectory, createSymbolicLink, createTempFile}
import org.junit.runner.RunWith
import org.scalatest.FreeSpec
import org.scalatest.Matchers._
import org.scalatest.junit.JUnitRunner

@RunWith(classOf[JUnitRunner])
final class WebXmlIT extends FreeSpec with ScalaSchedulerTest with JettyPluginJerseyTester with DirectoryListingTests {

  private lazy val configDir = controller.environment.configDirectory
  private lazy val baseDir = controller.environment.directory.getCanonicalFile / WebSuperdirectoryName
  private lazy val webDirectory = baseDir / WebDirectoryName

  override protected def checkedBeforeAll(): Unit = {
    controller.activateScheduler()
    prepareWebXml()
    createDirectory(webDirectory.getParentFile)
    createDirectory(webDirectory)
    injector.apply[PluginSubsystem].activatePlugin(classOf[JettyPlugin])
    super.checkedBeforeAll()
  }

  private def prepareWebXml(): Unit = {
    val webXmlFile = configDir / "web.xml"
    webXmlFile.contentString = webXmlFile.contentString.replace("{{SCHEDULER_WORK_URL}}", baseDir.toURI.toURL.toString)
  }

  "Web server delivers external files described in web.xml" in {
    val file = webDirectory / "test.html"
    file.contentString = "TEST"
    get[String](s"/jobscheduler/$WebDirectoryName/test.html") shouldEqual file.contentString
  }

  "Directory access" - {
    addDirectoryListingTests(WebDirectoryName)
  }

  if (!isWindows) {  // Windows mklink needs the special policy right "Local Policies\User Rights Assignment\Create symbolic links"
    "Symlink is accessible" in {
      val symlinkedFile = createTempFile("JettyPlugin-", ".tmp").toFile
      onClose { Files.delete(symlinkedFile) }
      symlinkedFile.contentString = "TEST"
      val name = "symlink.html"
      val symlinkFile = webDirectory / name
      createSymbolicLink(symlinkFile, symlinkedFile)
      onClose { Files.delete(symlinkFile) }
      get[String](s"/jobscheduler/$WebDirectoryName/$name") shouldEqual symlinkFile.contentString
    }
  }
}

object WebXmlIT {
  private val nonAsciiString =
    if (isWindows) " and Кирилица"
    else {
      /* Another workaround might be setting the "-Dfile.encoding=UTF-8" java parameter???
         java.nio.file.InvalidPathException: Malformed input or input contains unmappable characters: [...]/target/failsafe-reports/com.sos.scheduler.engine.plugins.jetty.tests.webxml.WebXmlIT/with blank and Кирилица
              at sun.nio.fs.UnixPath.encode(UnixPath.java:147)
              at sun.nio.fs.UnixPath.<init>(UnixPath.java:71)
              at sun.nio.fs.UnixFileSystem.getPath(UnixFileSystem.java:281)
              at java.io.File.toPath(File.java:2234)
              at com.sos.scheduler.engine.common.scalautil.FileUtils$implicits$.fileToPath(FileUtils.scala:14)
              at com.sos.scheduler.engine.plugins.jetty.tests.webxml.WebXmlIT.checkedBeforeAll(WebXmlIT.scala:29)
       */
      ""
    }
  private val WebSuperdirectoryName = "with blank" + nonAsciiString
  private val WebDirectoryName = "operations_gui"
}
