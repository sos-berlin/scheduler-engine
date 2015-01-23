package com.sos.scheduler.engine.plugins.jetty.configuration

import com.sos.scheduler.engine.common.scalautil.SideEffect._
import com.sos.scheduler.engine.common.xml.XmlUtils.loadXml
import com.sos.scheduler.engine.kernel.scheduler.SchedulerConfiguration
import com.sos.scheduler.engine.plugins.jetty.configuration.JettyConfiguration.{FixedTcpPortNumber, LazyRandomTcpPortNumber, WarEntry}
import com.sos.scheduler.engine.plugins.jetty.configuration.SchedulerConfigurationAdapter.jettyConfiguration
import com.sos.scheduler.engine.plugins.jetty.configuration.SchedulerConfigurationAdapterTest._
import java.io.File
import org.eclipse.jetty.util.security.Password
import org.junit.runner.RunWith
import org.mockito.Mockito._
import org.scalatest.FreeSpec
import org.scalatest.Matchers._
import org.scalatest.junit.JUnitRunner
import org.scalatest.mock.MockitoSugar.mock
import scala.collection.JavaConversions._

/**
 * @author Joacim Zschimmer
 */
@RunWith(classOf[JUnitRunner])
final class SchedulerConfigurationAdapterTest extends FreeSpec {

  private val httpPortSchedulerConfiguration = mockSchedulerConfiguration(httpPort = Some(1111))
  private val emptySchedulerConfiguration = mockSchedulerConfiguration()

  "HTTP port" - {
    "No port" in {
      val elem = <plugin.config/>
      jettyConfiguration(toDomElement(elem), emptySchedulerConfiguration).portOption shouldEqual None
    }

    "-http-port=1111" in {
      val elem = <plugin.config/>
      jettyConfiguration(toDomElement(elem), httpPortSchedulerConfiguration).portOption shouldEqual Some(FixedTcpPortNumber(1111))
    }

    "XML attribute port=2222" in {
      val elem = <plugin.config port="2222"/>
      jettyConfiguration(toDomElement(elem), emptySchedulerConfiguration).portOption shouldEqual Some(FixedTcpPortNumber(2222))
    }

    "-http-port=1111 and port=2222 should be rejected" in {
      val elem = <plugin.config port="2222"/>
      intercept[IllegalArgumentException] {
        jettyConfiguration(toDomElement(elem), httpPortSchedulerConfiguration).portOption
      }
    }

    "XML attribute port=TEST" in {
      val elem = <plugin.config port="TEST"/>
      jettyConfiguration(toDomElement(elem), emptySchedulerConfiguration).portOption.get shouldBe a [LazyRandomTcpPortNumber]
    }
  }

  "XML configuration with loginService and web archives" in {
    val elem =
      <plugin.config>
        <loginService>
          <logins>
            <login name="A-NAME" password="A-PASSWORD" roles="ADMINISTRATOR"/>
            <login name="B-NAME" password="B-PASSWORD" roles=" X  Y "/>
          </logins>
        </loginService>
        <webContexts>
          <warWebContext contextPath="/A" war="A.war"/>
          <warWebContext contextPath="/B" war="B.war"/>
        </webContexts>
      </plugin.config>

    val conf = jettyConfiguration(toDomElement(elem), emptySchedulerConfiguration)
    val loginService = conf.loginServiceOption.get.asInstanceOf[PluginLoginService]

    val aUser = loginService.getUsers.get("A-NAME")
    aUser.getSubject.getPrivateCredentials(classOf[Password]).toSet shouldEqual Set(new Password("A-PASSWORD"))
    aUser.isUserInRole("ADMINISTRATOR", null) shouldBe true
    aUser.isUserInRole("X", null) shouldBe false

    val bUser = loginService.getUsers.get("B-NAME")
    bUser.getSubject.getPrivateCredentials(classOf[Password]).toSet shouldEqual Set(new Password("B-PASSWORD"))
    bUser.isUserInRole("X", null) shouldBe true
    bUser.isUserInRole("Y", null) shouldBe true
    bUser.isUserInRole("", null) shouldBe false
    bUser.isUserInRole(" ", null) shouldBe false

    conf.wars shouldEqual List(WarEntry("/A", new File("A.war")), WarEntry("/B", new File("B.war")))
  }
}

private object SchedulerConfigurationAdapterTest {
  private def toDomElement(e: xml.Elem) = loadXml(e.toString()).getDocumentElement

  private def mockSchedulerConfiguration(httpPort: Option[Int] = None): SchedulerConfiguration =
    mock[SchedulerConfiguration] sideEffect { o ⇒
      when(o.httpPortOption) thenReturn httpPort
      when(o.webDirectoryUrlOption) thenReturn None
    }
}
