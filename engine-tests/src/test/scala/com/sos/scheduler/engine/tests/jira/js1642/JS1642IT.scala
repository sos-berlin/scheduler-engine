package com.sos.scheduler.engine.tests.jira.js1642

import com.google.common.io.Files.touch
import com.sos.scheduler.engine.client.api.SchedulerClient
import com.sos.scheduler.engine.client.web.StandardWebSchedulerClient
import com.sos.scheduler.engine.common.scalautil.Closers.implicits._
import com.sos.scheduler.engine.common.scalautil.FileUtils.implicits._
import com.sos.scheduler.engine.common.scalautil.Futures.implicits._
import com.sos.scheduler.engine.common.sprayutils.JsObjectMarshallers._
import com.sos.scheduler.engine.common.time.Stopwatch
import com.sos.scheduler.engine.data.filebased.FileBasedState
import com.sos.scheduler.engine.data.job.TaskId
import com.sos.scheduler.engine.data.jobchain.JobChainPath
import com.sos.scheduler.engine.data.order.{OrderKey, OrderOverview, OrderState, OrderStepStartedEvent}
import com.sos.scheduler.engine.data.scheduler.{SchedulerId, SchedulerState}
import com.sos.scheduler.engine.data.xmlcommands.{ModifyOrderCommand, OrderCommand}
import com.sos.scheduler.engine.kernel.DirectSchedulerClient
import com.sos.scheduler.engine.kernel.variable.VariableSet
import com.sos.scheduler.engine.plugins.jetty.test.JettyPluginTests._
import com.sos.scheduler.engine.test.EventBusTestFutures.implicits.RichEventBus
import com.sos.scheduler.engine.test.json.JsonRegexMatcher._
import com.sos.scheduler.engine.test.scalatest.ScalaSchedulerTest
import com.sos.scheduler.engine.tests.jira.js1642.JS1642IT._
import java.nio.file.Files.deleteIfExists
import java.time.Instant
import java.time.Instant._
import org.junit.runner.RunWith
import org.scalatest.FreeSpec
import org.scalatest.Matchers._
import org.scalatest.junit.JUnitRunner
import scala.collection.mutable
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
  private lazy val schedulerUri = s"http://127.0.0.1:${jettyPortNumber(injector)}"
  private lazy val webSchedulerClient = new StandardWebSchedulerClient(schedulerUri).closeWithCloser
  private lazy val barrierFile = testEnvironment.tmpDirectory / "TEST-BARRIER"
  private implicit lazy val executionContext = instance[ExecutionContext]
  private val orderKeyToTaskId = mutable.Map[OrderKey, TaskId]()

  override protected def onSchedulerActivated() = {
    super.onSchedulerActivated()
  }

  override def afterAll(): Unit = {
    deleteIfExists(barrierFile)
    super.afterAll()
  }

  private val setting = List[(String, () ⇒ SchedulerClient)](
    "DirectSchedulerClient" → { () ⇒ directSchedulerClient },
    "WebSchedulerClient" → { () ⇒ webSchedulerClient })

  "Prepare tests" in {
    val variableSet = instance[VariableSet]
    variableSet(TestJob.BarrierFileVariableName) = barrierFile.toString
    touch(barrierFile)
    scheduler executeXml OrderCommand(aAdHocOrderKey, at = Some(OrderStartAt), suspended = Some(true))
    for ((orderKey, expectedTaskId) ← ProcessableOrderKeys.zipWithIndex map { case (o, i) ⇒ o → (TaskId.First + i) }) {
      val event = eventBus.awaitingKeyedEvent[OrderStepStartedEvent](orderKey) {
        scheduler executeXml ModifyOrderCommand(orderKey, suspended = Some(false))
      }
      assert(event.taskId == expectedTaskId)
      orderKeyToTaskId(orderKey) = event.taskId
    }
  }

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
      assert(orders.toVector.sortBy { _.orderKey } == ExpectedOrderOverviews.sortBy { _.orderKey })
    }

    "orderOverviews speed" in {
      Stopwatch.measureTime(100, s""""orderOverviews with $OrderCount orders"""") {
        client.orderOverviews await TestTimeout
      }
    }
  }

  "StandardWebSchedulerClient in Java" in {
    SchedulerClientJavaTester.run(schedulerUri)
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
      val orderOverviews = webSchedulerClient.get[JsArray](_.order.overviews) await TestTimeout

      def path(o: JsValue) = o.asJsObject.fields("path").asInstanceOf[JsString].value   // The array's ordering is not assured.
      assert(orderOverviews.elements.sortBy(path) == ExpectedOrderOverviewsJson.elements.sortBy(path))
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
  private val OrderStartAt = Instant.parse("2038-01-01T11:22:33Z")

  private val aJobChainPath = JobChainPath("/aJobChain")
  private val a1OrderKey = aJobChainPath orderKey "1"
  private val a2OrderKey = aJobChainPath orderKey "2"
  private val aAdHocOrderKey = aJobChainPath orderKey "AD-HOC"

  private val bJobChainPath = JobChainPath("/bJobChain")
  private val b1OrderKey = bJobChainPath orderKey "1"

  private val aaJobChainPath = JobChainPath("/aFolder/a-aJobChain")
  private val aa1OrderKey = aaJobChainPath orderKey "1"
  private val aa2OrderKey = aaJobChainPath orderKey "2"

  private val abJobChainPath = JobChainPath("/aFolder/a-bJobChain")
  private val ab1OrderKey = abJobChainPath orderKey "1"

  private val OrderKeys = List(a1OrderKey, a2OrderKey, b1OrderKey, aAdHocOrderKey, aa1OrderKey, aa2OrderKey, ab1OrderKey)
  private val ProcessableOrderKeys = List(a1OrderKey, a2OrderKey, b1OrderKey)

  private val ExpectedOrderOverviews = Vector(
    OrderOverview(
      a1OrderKey,
      FileBasedState.active,
      OrderState("100"),
      nextStepAt = Some(EPOCH),
      taskId = Some(TaskId.First)),
    OrderOverview(
      a2OrderKey,
      FileBasedState.active,
      OrderState("100"),
      nextStepAt = Some(EPOCH),
      taskId = Some(TaskId.First + 1)),
    OrderOverview(
      b1OrderKey,
      FileBasedState.active,
      OrderState("100"),
      nextStepAt = Some(EPOCH),
      taskId = Some(TaskId.First + 2)),
    OrderOverview(
      aAdHocOrderKey,
      FileBasedState.notInitialized,
      OrderState("100"),
      nextStepAt = Some(OrderStartAt),
      isSuspended = true),
    OrderOverview(
      aa1OrderKey,
      FileBasedState.active,
      OrderState("100"),
      nextStepAt = Some(EPOCH)),
    OrderOverview(
      aa2OrderKey,
      FileBasedState.active,
      OrderState("100"),
      nextStepAt = Some(EPOCH)),
    OrderOverview(
      ab1OrderKey,
      FileBasedState.active,
      OrderState("100"),
      nextStepAt = Some(EPOCH)))

  private val OrderCount = ExpectedOrderOverviews.size

  private val ExpectedOrderOverviewsJson = """[
    {
      "path": "/aJobChain,1",
      "orderState": "100",
      "nextStepAt": "1970-01-01T00:00:00Z",
      "fileBasedState": "active",
      "taskId": "3",
      "isSuspended": false,
      "isOnBlacklist": false
    },
    {
      "path": "/aJobChain,2",
      "orderState": "100",
      "nextStepAt": "1970-01-01T00:00:00Z",
      "fileBasedState": "active",
      "taskId": "4",
      "isSuspended": false,
      "isOnBlacklist": false
    },
    {
      "path": "/bJobChain,1",
      "orderState": "100",
      "nextStepAt": "1970-01-01T00:00:00Z",
      "fileBasedState": "active",
      "taskId": "5",
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
    },
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
    }
  ]""".parseJson.asInstanceOf[JsArray]
}
