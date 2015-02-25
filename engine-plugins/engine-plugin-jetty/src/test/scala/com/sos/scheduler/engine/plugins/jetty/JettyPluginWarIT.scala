package com.sos.scheduler.engine.plugins.jetty

import com.google.common.io.Resources
import com.sos.scheduler.engine.common.scalautil.FileUtils.implicits._
import com.sos.scheduler.engine.common.scalautil.Logger
import com.sos.scheduler.engine.common.scalautil.xmls.ScalaXmls.implicits._
import com.sos.scheduler.engine.common.utils.JavaResource
import com.sos.scheduler.engine.plugins.jetty.JettyPluginWarIT._
import com.sos.scheduler.engine.plugins.jetty.test.HttpVerbRestrictionTester._
import com.sos.scheduler.engine.plugins.jetty.test.{HttpVerbRestrictionTester, JettyPluginJerseyTester}
import com.sos.scheduler.engine.plugins.webservice.tests.Tests
import com.sos.scheduler.engine.test.configuration.TestConfiguration
import com.sos.scheduler.engine.test.scalatest.ScalaSchedulerTest
import com.sos.scheduler.engine.test.util.IDE
import java.io.File
import org.junit.runner.RunWith
import org.scalatest.FreeSpec
import org.scalatest.Matchers._
import org.scalatest.junit.JUnitRunner
import scala.annotation.tailrec

@RunWith(classOf[JUnitRunner])
final class JettyPluginWarIT extends FreeSpec with ScalaSchedulerTest with JettyPluginJerseyTester {

  override lazy val testConfiguration = TestConfiguration(testClass = getClass, testPackage = Some(Tests.testPackage))

  override protected def onBeforeSchedulerActivation(): Unit = {
    (testEnvironment.configDirectory / "scheduler.xml").xml = generateSchedulerConfig(warFile())
  }

  "testwar.html" in {
    get[String](s"$ContextPath/testwar.html").trim shouldEqual "<html><body>TEST</body></html>"
  }

  "TestServlet" in {
    get[String](s"$ContextPath/TEST").trim shouldEqual "TestServlet"
  }

  "HTTP Verbs" in {
    new HttpVerbRestrictionTester(webResource).checkPathForVerbs(s"$ContextPath/TEST", GetServletMethods)
  }
}

private object JettyPluginWarIT {
  private val ContextPath = "/TESTWAR"
  private val WarFilename = "engine-testwar.war"
  private val WarRelativePath = s"engine/testwar/target/$WarFilename"
  private val logger = Logger(getClass)

  private def generateSchedulerConfig(warFile: File) =
    <spooler>
      <config>
        <plugins>
          <plugin java_class="com.sos.scheduler.engine.plugins.jetty.JettyPlugin">
            <plugin.config port="TEST">
              <webContexts>
                <warWebContext contextPath={ContextPath} war={warFile.getPath}/>
              </webContexts>
            </plugin.config>
          </plugin>
        </plugins>
      </config>
    </spooler>

  private def warFile(): File = {
    if (IDE.isRunningUnderIDE)
      forIdeFindWarFile()
    else
      new File(JavaResource(WarFilename).url.toURI)
  }

  /** IntelliJ kopiert nicht die engine-testwar.war, wie in der pom.xml definiert. Deshalb suchen wir sie selbst. */
  private def forIdeFindWarFile(): File = {
    val workingDir = new File(".").getAbsoluteFile
    @tailrec
    def f(baseDir: File): File =
      baseDir match {
        case d if (d / WarRelativePath).exists ⇒ d / WarRelativePath
        case d if (d != null) && (d / "pom.xml").exists ⇒ f(d.getParentFile)
        case _ ⇒ sys.error(s"$WarRelativePath is missing, all Maven directories up from $workingDir searched, we possibly do not run under Maven?")
      }
    val result = f(workingDir).getCanonicalFile
    logger.debug(s"Found $result")
    logger.warn(s"Using $WarFilename detected in directory ${result.getParent}")
    result
  }
}
