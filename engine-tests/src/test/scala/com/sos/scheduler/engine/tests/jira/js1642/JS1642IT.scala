package com.sos.scheduler.engine.tests.jira.js1642

import com.google.common.io.Files.touch
import com.sos.scheduler.engine.client.api.SchedulerClient
import com.sos.scheduler.engine.client.web.StandardWebSchedulerClient
import com.sos.scheduler.engine.common.convert.ConvertiblePartialFunctions.ImplicitConvertablePF
import com.sos.scheduler.engine.common.scalautil.Closers.implicits._
import com.sos.scheduler.engine.common.scalautil.FileUtils.implicits._
import com.sos.scheduler.engine.common.scalautil.Futures.implicits._
import com.sos.scheduler.engine.common.scalautil.Logger
import com.sos.scheduler.engine.common.scalautil.xmls.SafeXML
import com.sos.scheduler.engine.common.sprayutils.JsObjectMarshallers._
import com.sos.scheduler.engine.common.time.Stopwatch
import com.sos.scheduler.engine.data.compounds.OrdersFullOverview
import com.sos.scheduler.engine.data.filebased.FileBasedState
import com.sos.scheduler.engine.data.job.{JobOverview, JobPath, JobState, ProcessClassOverview, TaskId, TaskOverview, TaskState}
import com.sos.scheduler.engine.data.jobchain.{EndNodeOverview, JobChainDetails, JobChainNodeAction, JobChainOverview, JobChainPath, JobChainQuery, SimpleJobNodeOverview}
import com.sos.scheduler.engine.data.order.{OrderKey, OrderOverview, OrderQuery, OrderSourceType, OrderState, OrderStepStartedEvent}
import com.sos.scheduler.engine.data.processclass.ProcessClassPath
import com.sos.scheduler.engine.data.scheduler.{SchedulerId, SchedulerState}
import com.sos.scheduler.engine.data.xmlcommands.{ModifyOrderCommand, OrderCommand}
import com.sos.scheduler.engine.kernel.DirectSchedulerClient
import com.sos.scheduler.engine.kernel.async.SchedulerThreadFutures.inSchedulerThread
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
import scala.collection.{immutable, mutable}
import scala.concurrent.ExecutionContext
import spray.http.MediaTypes.{`text/html`, `text/richtext`}
import spray.http.StatusCodes.{InternalServerError, NotAcceptable, NotFound}
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
  private implicit lazy val executionContext = instance[ExecutionContext]
  private val orderKeyToTaskId = mutable.Map[OrderKey, TaskId]()

  private object barrier {
    lazy val file = testEnvironment.tmpDirectory / "TEST-BARRIER"

    def touchFile() = {
      val variableSet = instance[VariableSet]
      variableSet(TestJob.BarrierFileVariableName) = file.toString
      touch(file)
      onClose { deleteIfExists(file) }
    }
  }

  private val setting = List[(String, () ⇒ SchedulerClient)](
    "DirectSchedulerClient" → { () ⇒ directSchedulerClient },
    "WebSchedulerClient" → { () ⇒ webSchedulerClient })

  "Prepare tests" in {
    barrier.touchFile()
    scheduler executeXml OrderCommand(aAdHocOrderKey, at = Some(OrderStartAt), suspended = Some(true))
    startOrderProcessing()
  }

  private def startOrderProcessing() = {
    val expectedTaskIds = ProcessableOrderKeys.indices map { i ⇒ TaskId.First + i }
    for ((orderKey, expectedTaskId) ← ProcessableOrderKeys zip expectedTaskIds) {
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
      assert(overview == (directSchedulerClient.overview await  TestTimeout).copy(instant = overview.instant))
      assert(overview.schedulerId == SchedulerId("test"))
      assert(overview.state == SchedulerState.running)
    }

    "orderOverviews" in {
      val orders = client.orderOverviews await TestTimeout
      assert(orders == (directSchedulerClient.orderOverviews() await TestTimeout))
      assert(orders.toVector.sorted == ExpectedOrderOverviews)
    }

    "orderOverviews speed" in {
      Stopwatch.measureTime(50, s""""orderOverviews with $OrderCount orders"""") {
        client.orderOverviews await TestTimeout
      }
    }

    "ordersFullOverview" in {
      val fullOverview = client.ordersFullOverview await TestTimeout
      assert(fullOverview == (directSchedulerClient.ordersFullOverview() await TestTimeout))
      assert(sortOrdersFullOverview(fullOverview) == ExpectedOrderFullOverview)
    }

    "ordersFullOverview isSuspended" in {
      val orderQuery = OrderQuery(isSuspended = Some(true))
      val fullOverview = client.ordersFullOverview(orderQuery) await TestTimeout
      assert(fullOverview == (directSchedulerClient.ordersFullOverview(orderQuery) await TestTimeout))
      assert(sortOrdersFullOverview(fullOverview) == ExpectedOrderFullOverview.copy(
        orders = ExpectedOrderFullOverview.orders filter { _.isSuspended },
        usedTasks = Nil,
        usedJobs = Nil,
        usedProcessClasses = Nil))
    }

    "ordersFullOverview query /aJobChain" in {
      val orderQuery = OrderQuery(jobChainQuery = JobChainQuery("/aJobChain"))
      val fullOverview = client.ordersFullOverview(orderQuery) await TestTimeout
      assert(fullOverview == (directSchedulerClient.ordersFullOverview(orderQuery) await TestTimeout))
      assert((fullOverview.orders map { _.orderKey }).toSet == Set(a1OrderKey, a2OrderKey, aAdHocOrderKey))
    }

    "ordersFullOverview query /aJobChain/ returns nothing" in {
      val orderQuery = OrderQuery(jobChainQuery = JobChainQuery("/aJobChain/"))
      val fullOverview = client.ordersFullOverview(orderQuery) await TestTimeout
      assert(fullOverview == (directSchedulerClient.ordersFullOverview(orderQuery) await TestTimeout))
      assert(fullOverview.orders.isEmpty)
    }

    "ordersFullOverview query /xFolder/" in {
      val orderQuery = OrderQuery(jobChainQuery = JobChainQuery("/xFolder/"))
      val fullOverview = client.ordersFullOverview(orderQuery) await TestTimeout
      assert(fullOverview == (directSchedulerClient.ordersFullOverview(orderQuery) await TestTimeout))
      assert((fullOverview.orders map { _.orderKey }).toSet == Set(xa1OrderKey, xa2OrderKey, xb1OrderKey))
    }

    "ordersFullOverview query /xFolder returns nothing" in {
      val orderQuery = OrderQuery(jobChainQuery = JobChainQuery("/xFolder"))
      val fullOverview = client.ordersFullOverview(orderQuery) await TestTimeout
      assert(fullOverview == (directSchedulerClient.ordersFullOverview(orderQuery) await TestTimeout))
      assert(fullOverview.orders.isEmpty)
    }

    "jobChainOverview" in {
      val query = JobChainQuery("/xFolder/")
      val jobChainOverviews: immutable.Seq[JobChainOverview] = client.jobChainOverviews(query) await TestTimeout
      assert(jobChainOverviews == (directSchedulerClient.jobChainOverviews(query) await TestTimeout))
      assert(jobChainOverviews.toSet == Set(
        JobChainOverview(
          xaJobChainPath,
          FileBasedState.active),
        JobChainOverview(
          xbJobChainPath,
          FileBasedState.active)))
    }

    "jobChainOverviews" in {
      val jobChainDetails: JobChainDetails = client.jobChainDetails(xaJobChainPath) await TestTimeout
      assert(jobChainDetails == (directSchedulerClient.jobChainDetails(xaJobChainPath) await TestTimeout))
      assert(jobChainDetails.copy(fileModifiedAt = None, sourceXml = None) ==
        JobChainDetails(
          xaJobChainPath,
          FileBasedState.active,
          Some(testEnvironment.fileFromPath(xaJobChainPath)),
          fileModifiedAt = None,
          sourceXml = None,
          List(
            SimpleJobNodeOverview(
              orderState = OrderState("100"),
              nextState = OrderState("END"),
              errorState = OrderState(""),
              JobChainNodeAction.process,
              JobPath("/xFolder/test"),
              orderCount = 2),
            EndNodeOverview(
              orderState = OrderState("END")))))
      assert(jobChainDetails.sourceXml.get startsWith "<job_chain ")
    }

    def sortOrdersFullOverview(o: OrdersFullOverview) = o.copy(
      orders = o.orders.sorted,
      usedTasks = o.usedTasks.sorted,
      usedJobs = o.usedJobs.sorted,
      usedProcessClasses = o.usedProcessClasses.sorted)

    "ordersFullOverview speed" in {
      Stopwatch.measureTime(50, "ordersFullOverview") {
        client.ordersFullOverview await TestTimeout
      }
    }

    "jobChainView" in {
      assert((client.jobChainOverviews(JobChainQuery.All) await TestTimeout).toSet == Set(
        JobChainOverview(aJobChainPath, FileBasedState.active),
        JobChainOverview(bJobChainPath, FileBasedState.active),
        JobChainOverview(xaJobChainPath, FileBasedState.active),
        JobChainOverview(xbJobChainPath, FileBasedState.active)))
    }

    "<show_state>" in {
      val response = client.executeXml("<show_state/>") map SafeXML.loadString await TestTimeout
      val state = response \ "answer" \ "state"
      assert((state \ "@state").toString == "running")
      assert((state \ "@ip_address").toString == "127.0.0.1")
    }

    "Speed test <show_state>" in {
      Stopwatch.measureTime(10, "<show_state what='orders'>") {
        client.executeXml("<show_state what='orders'/>") map SafeXML.loadString await TestTimeout
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
      val orderOverviews = webSchedulerClient.get[JsArray](_.order.overviews()) await TestTimeout
      // The array's ordering is not assured.
      assert(sortArray(orderOverviews, "path") == ExpectedOrderOverviewsJsArray)
    }

    "ordersFullOverview" in {
      val fullOverview = webSchedulerClient.get[JsObject](_.order.fullOverview()) await TestTimeout
      // The array's ordering is not assured.
      val orderedFullOverview = JsObject(fullOverview.fields ++ Map(
        sortArrayField(fullOverview, "orders", "path"),
        sortArrayField(fullOverview, "usedTasks", "id"),
        sortArrayField(fullOverview, "usedJobs", "path")))
      assert(orderedFullOverview == ExpectedOrdersFullOverviewJsObject)
    }

    def sortArrayField(jsObject: JsObject, arrayKey: String, key: String) =
      arrayKey → sortArray(jsObject.fields(arrayKey).asInstanceOf[JsArray], key)

    def sortArray(jsArray: JsArray, key: String) = JsArray(jsArray.elements.sortBy(selectString(key)))

    def selectString(key: String)(o: JsValue): String = o.asJsObject.fields(key).asInstanceOf[JsString].value
  }

  "Unknown Accept content type is rejected" - {
    "overview" in {
      intercept[UnsuccessfulResponseException] {
        webSchedulerClient.get[String](_.overview, accept = `text/richtext`) await TestTimeout
      }.response.status shouldEqual NotAcceptable
    }
  }

  "text/html" - {  // Inofficial
    "overview" in {
      val html = webSchedulerClient.get[String](_.overview, accept = `text/html`) await TestTimeout
      assert(html startsWith "<!DOCTYPE html")
      assert(html endsWith "</html>")
      assert(html contains "JobScheduler")
      assert(html contains "Started at")
    }

    "order.fullOverview" in {
      val html = webSchedulerClient.get[String](_.order.fullOverview(), accept = `text/html`) await TestTimeout
      assert(html startsWith "<!DOCTYPE html")
      assert(html endsWith "</html>")
      assert(html contains "JobScheduler")
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
    "jobscheduler/master/api/ERROR-500" in {
      intercept[UnsuccessfulResponseException] { webSchedulerClient.get[String](_.uriString(s"TEST/ERROR-500")) await TestTimeout }
        .response.status shouldEqual InternalServerError
    }

    "jobscheduler/master/api/OutOfMemoryError" in {
      intercept[UnsuccessfulResponseException] { webSchedulerClient.get[String](_.uriString("TEST/OutOfMemoryError")) await TestTimeout }
        .response.status shouldEqual InternalServerError
      webSchedulerClient.get[String](_.overview) await TestTimeout
    }

    "jobscheduler/master/api/UNKNOWN" in {
      intercept[UnsuccessfulResponseException] { webSchedulerClient.get[String](_.uriString("TEST/UNKNOWN")) await TestTimeout }
        .response.status shouldEqual NotFound
    }
  }

  "Speed test simple direct OrderOverview C++ access" in {
    inSchedulerThread {
      Stopwatch.measureTime(1000, "OrderOverview") {
        directSchedulerClient.orderOverviews().successValue
      }
    }
  }

  for (n ← sys.props.optionAs[Int]("JS1642IT")) {
    s"Speed test: OrdersFullOverview with $n orders, OrderQuery.All" - {
      s"(Add $n orders)" in {
        val stopwatch = new Stopwatch
        for (orderKey ← 1 to n map { i ⇒ aJobChainPath orderKey s"adhoc-$i" })
          controller.scheduler executeXml OrderCommand(orderKey)
        logger.info("Adding orders: " + stopwatch.itemsPerSecondString(n, "order"))
      }
      for ((testName, getClient) ← setting) {
        testName in {
          val stopwatch = new Stopwatch
          getClient().ordersFullOverview await TestTimeout
          logger.info(s"OrdersFullOverview $testName: " + stopwatch.itemsPerSecondString(n, "order"))
        }
      }
    }
  }
}

private object JS1642IT {
  private val logger = Logger(getClass)
  private val OrderStartAt = Instant.parse("2038-01-01T11:22:33Z")

  private val TestJobPath = JobPath("/test")

  private val aJobChainPath = JobChainPath("/aJobChain")
  private val a1OrderKey = aJobChainPath orderKey "1"
  private val a2OrderKey = aJobChainPath orderKey "2"
  private val aAdHocOrderKey = aJobChainPath orderKey "AD-HOC"

  private val bJobChainPath = JobChainPath("/bJobChain")
  private val b1OrderKey = bJobChainPath orderKey "1"

  private val xaJobChainPath = JobChainPath("/xFolder/x-aJobChain")
  private val xa1OrderKey = xaJobChainPath orderKey "1"
  private val xa2OrderKey = xaJobChainPath orderKey "2"

  private val xbJobChainPath = JobChainPath("/xFolder/x-bJobChain")
  private val xb1OrderKey = xbJobChainPath orderKey "1"

  private val ProcessableOrderKeys = Vector(a1OrderKey, a2OrderKey, b1OrderKey)

  private val ExpectedOrderOverviews = Vector(
    OrderOverview(
      a1OrderKey,
      FileBasedState.active,
      OrderSourceType.fileBased,
      OrderState("100"),
      nextStepAt = Some(EPOCH),
      taskId = Some(TaskId.First)),
    OrderOverview(
      a2OrderKey,
      FileBasedState.active,
      OrderSourceType.fileBased,
      OrderState("100"),
      nextStepAt = Some(EPOCH),
      taskId = Some(TaskId.First + 1)),
    OrderOverview(
      aAdHocOrderKey,
      FileBasedState.not_initialized,
      OrderSourceType.adHoc,
      OrderState("100"),
      nextStepAt = Some(OrderStartAt),
      isSuspended = true),
    OrderOverview(
      b1OrderKey,
      FileBasedState.active,
      OrderSourceType.fileBased,
      OrderState("100"),
      nextStepAt = Some(EPOCH),
      taskId = Some(TaskId.First + 2)),
    OrderOverview(
      xa1OrderKey,
      FileBasedState.active,
      OrderSourceType.fileBased,
      OrderState("100"),
      nextStepAt = Some(EPOCH)),
    OrderOverview(
      xa2OrderKey,
      FileBasedState.active,
      OrderSourceType.fileBased,
      OrderState("100"),
      nextStepAt = Some(EPOCH),
      isSuspended = true),
    OrderOverview(
      xb1OrderKey,
      FileBasedState.active,
      OrderSourceType.fileBased,
      OrderState("100"),
      nextStepAt = Some(EPOCH)))

  private val ExpectedOrderFullOverview = OrdersFullOverview(
    ExpectedOrderOverviews,
    Vector(
      TaskOverview(TaskId(3), TestJobPath, TaskState.running, ProcessClassPath.Default),
      TaskOverview(TaskId(4), TestJobPath, TaskState.running, ProcessClassPath.Default),
      TaskOverview(TaskId(5), TestJobPath, TaskState.running, ProcessClassPath.Default)),
    Vector(
      JobOverview(TestJobPath, FileBasedState.active, defaultProcessClass = None, JobState.running, isInPeriod = true, taskLimit = 10, usedTaskCount = 3)),
    Vector(
      ProcessClassOverview(ProcessClassPath.Default, FileBasedState.active, processLimit = 30, usedProcessCount = 3)))

  private val OrderCount = ExpectedOrderOverviews.size

  private val ExpectedOrderOverviewsJsArray: JsArray = """[
    {
      "path": "/aJobChain,1",
      "fileBasedState": "active",
      "sourceType": "fileBased",
      "orderState": "100",
      "nextStepAt": "1970-01-01T00:00:00Z",
      "taskId": "3",
      "isSuspended": false,
      "isBlacklisted": false
    },
    {
      "path": "/aJobChain,2",
      "fileBasedState": "active",
      "sourceType": "fileBased",
      "orderState": "100",
      "nextStepAt": "1970-01-01T00:00:00Z",
      "taskId": "4",
      "isSuspended": false,
      "isBlacklisted": false
    },
    {
      "path": "/aJobChain,AD-HOC",
      "fileBasedState": "not_initialized",
      "orderState": "100",
      "sourceType": "adHoc",
      "nextStepAt": "2038-01-01T11:22:33Z",
      "isSuspended": true,
      "isBlacklisted": false
    },
    {
      "path": "/bJobChain,1",
      "fileBasedState": "active",
      "orderState": "100",
      "sourceType": "fileBased",
      "nextStepAt": "1970-01-01T00:00:00Z",
      "taskId": "5",
      "isSuspended": false,
      "isBlacklisted": false
    },
    {
      "path": "/xFolder/x-aJobChain,1",
      "fileBasedState": "active",
      "sourceType": "fileBased",
      "orderState": "100",
      "nextStepAt": "1970-01-01T00:00:00Z",
      "isSuspended": false,
      "isBlacklisted": false
    },
    {
      "path": "/xFolder/x-aJobChain,2",
      "fileBasedState": "active",
      "sourceType": "fileBased",
      "orderState": "100",
      "nextStepAt": "1970-01-01T00:00:00Z",
      "isSuspended": true ,
      "isBlacklisted": false
    },
    {
      "path": "/xFolder/x-bJobChain,1",
      "fileBasedState": "active",
      "sourceType": "fileBased",
      "orderState": "100",
      "nextStepAt": "1970-01-01T00:00:00Z",
      "isSuspended": false,
      "isBlacklisted": false
    }
  ]""".parseJson.asInstanceOf[JsArray]

  private val ExpectedOrdersFullOverviewJsObject: JsObject = s"""{
    "orders": $ExpectedOrderOverviewsJsArray,
    "usedTasks": [
      {
        "id": "3",
        "job": "/test",
        "state": "running",
        "processClass": ""
      },
      {
        "id": "4",
        "job": "/test",
        "state": "running",
        "processClass": ""
      },
      {
        "id": "5",
        "job": "/test",
        "state": "running",
        "processClass": ""
      }
    ],
    "usedJobs": [
      {
        "path": "/test",
        "fileBasedState": "active",
        "taskLimit": 10,
        "state": "running",
        "isInPeriod": true,
        "usedTaskCount": 3
      }
    ],
    "usedProcessClasses": [
      {
        "path": "",
        "fileBasedState": "active",
        "processLimit": 30,
        "usedProcessCount": 3
      }
    ]
  }""".parseJson.asJsObject
}
