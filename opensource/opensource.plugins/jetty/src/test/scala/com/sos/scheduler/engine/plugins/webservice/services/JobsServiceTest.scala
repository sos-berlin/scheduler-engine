package com.sos.scheduler.engine.plugins.webservice.services

import com.google.inject.{AbstractModule, Guice}
import com.sos.scheduler.engine.common.scalautil.SideEffect._
import com.sos.scheduler.engine.data.job.JobPath
import com.sos.scheduler.engine.kernel.job.JobSubsystem
import com.sos.scheduler.engine.plugins.jetty.configuration.injection.JerseyModule
import com.sos.scheduler.engine.plugins.jetty.test.WebServiceTester
import javax.ws.rs.core.MediaType._
import org.junit.runner.RunWith
import org.mockito.Mockito._
import org.scalatest.Matchers._
import org.scalatest.junit.JUnitRunner
import org.scalatest.mock.MockitoSugar
import org.scalatest.{FreeSpec, BeforeAndAfterAll}
import spray.json._

@RunWith(classOf[JUnitRunner])
final class JobsServiceTest extends FreeSpec with BeforeAndAfterAll with MockitoSugar {

  private val mockedJobSubsystem = mock[JobSubsystem] sideEffect { o ⇒
    when(o.visiblePaths) thenReturn List(JobPath("/a"), JobPath("/b/c"))
  }

  private lazy val injector = Guice.createInjector(
    new JerseyModule,
    new AbstractModule {
      def configure() {
        bind(classOf[JobsService])
        bind(classOf[JobSubsystem]) toInstance mockedJobSubsystem
      }
    })
  private lazy val tester = new WebServiceTester(injector)

  override def beforeAll() {
    tester.start()
  }

  override def afterAll() {
    tester.close()
  }

  "Read job list" in {
    tester.webResource.path("/jobscheduler/engine/jobs").accept(APPLICATION_JSON_TYPE).get(classOf[String]).parseJson shouldEqual
      """[ "/a", "/b/c" ]""".parseJson
  }
}
