package com.sos.scheduler.engine.tests.jira.js1642

import com.sos.scheduler.engine.client.WebSchedulerClient
import com.sos.scheduler.engine.common.scalautil.AutoClosing.autoClosing
import com.sos.scheduler.engine.common.scalautil.Closers.implicits.RichClosersAutoCloseable
import com.sos.scheduler.engine.common.scalautil.Futures.implicits._
import com.sos.scheduler.engine.common.time.Stopwatch
import com.sos.scheduler.engine.data.scheduler.{SchedulerId, SchedulerState}
import com.sos.scheduler.engine.kernel.order.OrderSubsystem
import com.sos.scheduler.engine.plugins.jetty.test.JettyPluginTests._
import com.sos.scheduler.engine.test.json.JsonRegexMatcher._
import com.sos.scheduler.engine.test.scalatest.ScalaSchedulerTest
import com.sos.scheduler.engine.tests.jira.js1642.Provisioning._
import org.junit.runner.RunWith
import org.scalatest.FreeSpec
import org.scalatest.Matchers._
import org.scalatest.junit.JUnitRunner
import spray.http.StatusCodes.{InternalServerError, NotFound}
import spray.httpx.UnsuccessfulResponseException

@RunWith(classOf[JUnitRunner])
final class NewSchedulerClientIT extends FreeSpec with ScalaSchedulerTest with Provisioning {

  private lazy val port: Int = jettyPortNumber(injector)
  private lazy val schedulerUri = s"http://127.0.0.1:$port"
  private lazy val orderSubsystem = instance[OrderSubsystem]
  private lazy val client = WebSchedulerClient(schedulerUri).closeWithCloser

  "overview" in {
    assert(client.uris.overview == s"$schedulerUri/new/master/api")
    checkRegexJson(
      json = client.get[String](_.overview) await TestTimeout,
      patternMap = Map(
        "version" → """\d+\..+""".r,
        "versionCommitHash" → ".*".r,
        "startInstant" → AnyIsoTimestamp,
        "instant" → AnyIsoTimestamp,
        "schedulerId" → "test",
        "pid" → AnyInt,
        "state" → "running"))
    val overview = client.overview await TestTimeout
    assert(overview == (scheduler.overviewFuture await TestTimeout).copy(instant = overview.instant))
    assert(overview.schedulerId == SchedulerId("test"))
    assert(overview.state == SchedulerState.running)
  }

  "/new/master/api/order/" in {
    assert(client.uris.orderOverviews == s"$schedulerUri/new/master/api/order/")
    val orders = client.orderOverviews await TestTimeout
    assert(orders == (orderSubsystem.orderOverviews await TestTimeout))
  }

  "/new/master/api/order/ speed" in {
    Stopwatch.measureTime(100, s""""orderOverviews with $OrderCount orders"""") {
      client.orderOverviews await TestTimeout
    }
  }

  "new/master/api/ERROR-500" in {
    assert(client.uris.test.error500 == s"$schedulerUri/new/master/api/test/ERROR-500")
    intercept[UnsuccessfulResponseException] { client.get[String](_.test.error500) await TestTimeout }
      .response.status shouldEqual InternalServerError
  }

  "new/master/api/OutOfMemoryError" in {
    assert(client.uris.test.outOfMemoryError == s"$schedulerUri/new/master/api/test/OutOfMemoryError")
    intercept[UnsuccessfulResponseException] { client.get[String](_.test.outOfMemoryError) await TestTimeout }
      .response.status shouldEqual InternalServerError
    client.get[String](_.overview) await TestTimeout
  }

  "new/master/api/UNKNOWN" in {
    assert(client.uris.test.unknown == s"$schedulerUri/new/master/api/test/UNKNOWN")
    intercept[UnsuccessfulResponseException] { client.get[String](_.test.unknown) await TestTimeout }
      .response.status shouldEqual NotFound
  }

  "Java" in {
    autoClosing(new NewClientJavaTests(schedulerUri)) {
      _.test()
    }
  }

//  "/" in {
//    pending
//    val response = get[ClientResponse]("/")
//    response.getStatus shouldEqual TEMPORARY_REDIRECT.getStatusCode
//    response.getLocation shouldEqual new URI("z/gui/")
//  }
//
//  "/new" in {
//    pendingUntilFixed {  // Liefert Verzeichnis statt index.html
//      get[String]("/new/z") should include ("<html>")
//    }
//  }
//
//  "/new/" in {
//    pending
//    get[String]("/new/z/") should include ("<html>")
//  }
//
//  "/new/index.html" in {
//    pending
//    get[String]("/new/z/index.html") should include ("<html>")
//  }
//
//  "../ should be rejected" in {
//    //pendingUntilFixed {  // Spray scheint führende .. zu entfernen und andere zu verrechnen
//      intercept[UniformInterfaceException] { get[String]("/new/z/../index.html") }
//        .getResponse.getStatus shouldEqual NOT_FOUND.getStatusCode
//    //}
//  }
}
