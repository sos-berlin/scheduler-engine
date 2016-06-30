package com.sos.scheduler.engine.tests.jira.js1642

import com.sos.scheduler.engine.client.{SchedulerClient, StandardWebSchedulerClient}
import com.sos.scheduler.engine.common.scalautil.AutoClosing.autoClosing
import com.sos.scheduler.engine.common.scalautil.Closers.implicits.RichClosersAutoCloseable
import com.sos.scheduler.engine.common.scalautil.Futures.implicits._
import com.sos.scheduler.engine.common.sprayutils.JsObjectMarshallers._
import com.sos.scheduler.engine.common.time.Stopwatch
import com.sos.scheduler.engine.data.filebased.FileBasedState
import com.sos.scheduler.engine.data.jobchain.JobChainPath
import com.sos.scheduler.engine.data.order.{OrderOverview, OrderState}
import com.sos.scheduler.engine.data.scheduler.{SchedulerId, SchedulerState}
import com.sos.scheduler.engine.data.xmlcommands.OrderCommand
import com.sos.scheduler.engine.kernel.DirectSchedulerClient
import com.sos.scheduler.engine.plugins.jetty.test.JettyPluginTests._
import com.sos.scheduler.engine.test.json.JsonRegexMatcher._
import com.sos.scheduler.engine.test.scalatest.ScalaSchedulerTest
import com.sos.scheduler.engine.tests.jira.js1642.JS1642IT._
import java.time.Instant
import java.time.Instant._
import org.junit.runner.RunWith
import org.scalatest.FreeSpec
import org.scalatest.Matchers._
import org.scalatest.junit.JUnitRunner
import scala.concurrent.ExecutionContext
import spray.http.StatusCodes.{InternalServerError, NotFound}
import spray.httpx.UnsuccessfulResponseException
import spray.json._

/**
  * JS-1642 WebSchedulerClient and NewWebServicePlugin.
  *
  * @author Joacim Zschimmer
  */
@RunWith(classOf[JUnitRunner])
final class JS1642IT extends FreeSpec with ScalaSchedulerTest {
  private lazy val directSchedulerClient = instance[DirectSchedulerClient]

  private lazy val port: Int = jettyPortNumber(injector)
  private lazy val schedulerUri = s"http://127.0.0.1:$port"
  private lazy val webSchedulerClient = new StandardWebSchedulerClient(schedulerUri).closeWithCloser
  private implicit lazy val executinoContext = instance[ExecutionContext]

  override protected def onSchedulerActivated() = {
    scheduler executeXml OrderCommand(
      JobChainPath("/aJobChain") orderKey "AD-HOC",
      at = Some(OrderStartAt),
      suspended = Some(true))
    super.onSchedulerActivated()
  }

  private val setting = List[(String, () ⇒ SchedulerClient)](
    "DirectSchedulerClient" → { () ⇒ directSchedulerClient },
    "WebSchedulerClient" → { () ⇒ webSchedulerClient })

  for ((testGroup, getClient) ← setting) testGroup - {
    lazy val client = getClient()

    "overview" in {
      val overview = client.overview await TestTimeout
      assert(overview == (directSchedulerClient.overview await TestTimeout).copy(instant = overview.instant))
      assert(overview.schedulerId == SchedulerId("test"))
      assert(overview.state == SchedulerState.running)
    }

    "orderOverviews" in {
      val orders = client.orderOverviews await TestTimeout
      assert(orders == (directSchedulerClient.orderOverviews await TestTimeout))
      assert(orders.toVector.sortBy { _.path } == ExpectedOrderViews.sortBy { _.path })
    }

    "orderOverviews speed" in {
      Stopwatch.measureTime(1000, s""""orderOverviews with $OrderCount orders"""") {
        client.orderOverviews await TestTimeout
      }
    }
  }

  "StandardWebSchedulerClient in Java" in {
    autoClosing(new NewClientJavaTests(schedulerUri)) {
      _.test()
    }
  }

  "JSON" - {
    "overview" in {
      val overview = webSchedulerClient.get[JsObject](_.overview) await TestTimeout
      checkRegexJson(
        json = overview.toString,
        patternMap = Map(
          "version" → """\d+\..+""".r,
          "versionCommitHash" → ".*".r,
          "startInstant" → AnyIsoTimestamp,
          "instant" → AnyIsoTimestamp,
          "schedulerId" → "test",
          "pid" → AnyInt,
          "state" → "running"))
    }

    "orderOverviews" in {
      val orderOverviews = webSchedulerClient.get[JsArray](_.orderOverviews) await TestTimeout
      // The array's ordering is not assured.
      assert(orderOverviews == """[
        {
          "path": "/aFolder/a-aJobChain,1",
          "orderState": "100",
          "nextStepAt": "1970-01-01T00:00:00Z",
          "fileBasedState": "active",
          "isSuspended": false,
          "isOnBlacklist": false
        },
        {
          "path": "/aFolder/a-aJobChain,2",
          "orderState": "100",
          "nextStepAt": "1970-01-01T00:00:00Z",
          "fileBasedState": "active",
          "isSuspended": false,
          "isOnBlacklist": false
        },
        {
          "path": "/aFolder/a-bJobChain,1",
          "orderState": "100",
          "nextStepAt": "1970-01-01T00:00:00Z",
          "fileBasedState": "active",
          "isSuspended": false,
          "isOnBlacklist": false
        },
        {
          "path": "/bJobChain,1",
          "orderState": "100",
          "nextStepAt": "1970-01-01T00:00:00Z",
          "fileBasedState": "active",
          "isSuspended": false,
          "isOnBlacklist": false
        },
        {
          "path": "/aJobChain,2",
          "orderState": "100",
          "nextStepAt": "1970-01-01T00:00:00Z",
          "fileBasedState": "active",
          "isSuspended": false,
          "isOnBlacklist": false
        },
        {
          "path": "/aJobChain,1",
          "orderState": "100",
          "nextStepAt": "1970-01-01T00:00:00Z",
          "fileBasedState": "active",
          "isSuspended": false,
          "isOnBlacklist": false
        },
        {
          "path": "/aJobChain,AD-HOC",
          "orderState": "100",
          "nextStepAt": "2038-01-01T11:22:33Z",
          "fileBasedState": "notInitialized",
          "isSuspended": true,
          "isOnBlacklist": false
        }
      ]""".parseJson)
    }
  }

  "WebSchedulerClient.getJson" in {
    val jsonString = webSchedulerClient.getJson("api") await TestTimeout
    val jsObject = jsonString.parseJson.asJsObject
    val directOverview = directSchedulerClient.overview await TestTimeout
    assert(jsObject.fields("version") == JsString(directOverview.version))
    assert(jsObject.fields("versionCommitHash") == JsString(directOverview.versionCommitHash))
  }

  "Web service error behavior" - {
    "new/master/api/ERROR-500" in {
      assert(webSchedulerClient.uris.test.error500 == s"$schedulerUri/new/master/api/test/ERROR-500")
      intercept[UnsuccessfulResponseException] { webSchedulerClient.get[String](_.test.error500) await TestTimeout }
        .response.status shouldEqual InternalServerError
    }

    "new/master/api/OutOfMemoryError" in {
      assert(webSchedulerClient.uris.test.outOfMemoryError == s"$schedulerUri/new/master/api/test/OutOfMemoryError")
      intercept[UnsuccessfulResponseException] { webSchedulerClient.get[String](_.test.outOfMemoryError) await TestTimeout }
        .response.status shouldEqual InternalServerError
      webSchedulerClient.get[String](_.overview) await TestTimeout
    }

    "new/master/api/UNKNOWN" in {
      assert(webSchedulerClient.uris.test.unknown == s"$schedulerUri/new/master/api/test/UNKNOWN")
      intercept[UnsuccessfulResponseException] { webSchedulerClient.get[String](_.test.unknown) await TestTimeout }
        .response.status shouldEqual NotFound
    }
  }
}

private object JS1642IT {
  final val OrderStartAt = Instant.parse("2038-01-01T11:22:33Z")

  val ExpectedOrderViews = Vector(
    OrderOverview(
      JobChainPath("/aJobChain") orderKey "1",
      FileBasedState.active,
      OrderState("100"),
      nextStepAt = Some(EPOCH)),
    OrderOverview(
      JobChainPath("/aJobChain") orderKey "2",
      FileBasedState.active,
      OrderState("100"),
      nextStepAt = Some(EPOCH)),
    OrderOverview(
      JobChainPath("/aJobChain") orderKey "AD-HOC",
      FileBasedState.notInitialized,
      OrderState("100"),
      nextStepAt = Some(OrderStartAt),
      isSuspended = true),
    OrderOverview(
      JobChainPath("/bJobChain") orderKey "1",
      FileBasedState.active,
      OrderState("100"),
      nextStepAt = Some(EPOCH)),
    OrderOverview(
      JobChainPath("/aFolder/a-aJobChain") orderKey "1",
      FileBasedState.active,
      OrderState("100"),
      nextStepAt = Some(EPOCH)),
    OrderOverview(
      JobChainPath("/aFolder/a-aJobChain") orderKey "2",
      FileBasedState.active,
      OrderState("100"),
      nextStepAt = Some(EPOCH)),
    OrderOverview(
      JobChainPath("/aFolder/a-bJobChain") orderKey "1",
      FileBasedState.active,
      OrderState("100"),
      nextStepAt = Some(EPOCH)))

  val OrderCount = ExpectedOrderViews.size
}
