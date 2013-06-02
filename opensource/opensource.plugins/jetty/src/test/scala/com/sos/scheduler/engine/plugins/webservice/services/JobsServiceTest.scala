package com.sos.scheduler.engine.plugins.webservice.services

import com.google.common.collect.ImmutableList
import com.google.inject.{AbstractModule, Guice}
import com.sos.scheduler.engine.common.scalautil.ModifiedBy.modifiedBy
import com.sos.scheduler.engine.kernel.job.JobSubsystem
import com.sos.scheduler.engine.plugins.jetty.configuration.Config
import com.sos.scheduler.engine.plugins.jetty.configuration.injection.JerseyModule
import com.sos.scheduler.engine.plugins.jetty.tests.commons.WebServiceTester
import com.sun.jersey.api.client.GenericType
import javax.ws.rs.core.MediaType._
import org.junit.runner.RunWith
import org.mockito.Mockito._
import org.scalatest.junit.JUnitRunner
import org.scalatest.matchers.ShouldMatchers._
import org.scalatest.mock.MockitoSugar
import org.scalatest.{BeforeAndAfterAll, FunSuite}

@RunWith(classOf[JUnitRunner])
final class JobsServiceTest extends FunSuite with BeforeAndAfterAll with MockitoSugar {

  private val mockedJobSubsystem = mock[JobSubsystem] modifiedBy { o =>
    when(o.getVisibleNames) thenReturn ImmutableList.of("a", "b/c")
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
  private lazy val jobsResource = tester.webResource.path(Config.enginePrefixPath).path("jobs")

  override def beforeAll() {
    tester.start()
  }

  override def afterAll() {
    tester.close()
  }

  test("Read job list") {
    jobsResource.accept(APPLICATION_JSON_TYPE).get(new GenericType[Set[String]]() {}) should equal (Set("a", "b/c"))
  }
}
