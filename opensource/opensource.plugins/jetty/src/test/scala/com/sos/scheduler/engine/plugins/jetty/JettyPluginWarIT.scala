package com.sos.scheduler.engine.plugins.jetty

import com.sos.scheduler.engine.common.scalautil.Logger
import com.sos.scheduler.engine.common.scalautil.ScalaXmls.implicits._
import com.sos.scheduler.engine.plugins.jetty.JettyPluginWarIT._
import com.sos.scheduler.engine.plugins.jetty.test.JettyPluginJerseyTester
import com.sos.scheduler.engine.plugins.webservice.tests.Tests
import com.sos.scheduler.engine.test.configuration.TestConfiguration
import com.sos.scheduler.engine.test.scala.ScalaSchedulerTest
import java.io.File
import org.junit.runner.RunWith
import org.scalatest.FreeSpec
import org.scalatest.Matchers._
import org.scalatest.junit.JUnitRunner
import scala.annotation.tailrec

@RunWith(classOf[JUnitRunner])
final class JettyPluginWarIT extends FreeSpec with ScalaSchedulerTest with JettyPluginJerseyTester {

  override lazy val testConfiguration = TestConfiguration(testClass = getClass, testPackage = Some(Tests.testPackage))

  override protected def onBeforeSchedulerActivation() {
    val warFile = findWarFile()
    new File(testEnvironment.configDirectory, "scheduler.xml").xml = schedulerConfigElem(warFile)
  }

  ".war" - {
    "testwar.html" in {
      webResource.path(s"$ContextPath/testwar.html").get(classOf[String]).trim shouldEqual "<html><body>TEST</body></html>"
    }

    "TestServlet" in {
      webResource.path(s"$ContextPath/TEST").get(classOf[String]).trim shouldEqual "TestServlet"
    }
  }
}

private object JettyPluginWarIT {
  private val ContextPath = "/TESTWAR"
  private val WarRelativePath = "engine/engine-testwar/target/engine-testwar.war"
  private val logger = Logger(getClass)

  private def schedulerConfigElem(warFile: File) =
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

  private def findWarFile(): File = {
    val workingDir = new File(".").getAbsoluteFile
    @tailrec
    def f(baseDir: File): File =
      baseDir match {
        case o if new File(baseDir, WarRelativePath).exists ⇒ new File(baseDir, WarRelativePath)
        case o if (o != null) && new File(o, "pom.xml").exists ⇒ f(o.getParentFile)
        case _ ⇒ sys.error(s"$WarRelativePath is missing, all Maven directories up from $workingDir searched, we possibly do not run under Maven?")
      }
    val result = f(workingDir).getCanonicalFile
    logger.debug(s"Found $result")
    result
  }
}
