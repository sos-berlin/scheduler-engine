package com.sos.scheduler.engine.tests.jira.js1642

import akka.util.ByteString
import com.google.common.io.Files.touch
import com.sos.scheduler.engine.base.sprayjson.JsonRegexMatcher._
import com.sos.scheduler.engine.client.api.SchedulerClient
import com.sos.scheduler.engine.client.web.StandardWebSchedulerClient
import com.sos.scheduler.engine.common.scalautil.Closers.implicits._
import com.sos.scheduler.engine.common.scalautil.FileUtils.implicits._
import com.sos.scheduler.engine.common.scalautil.Futures.implicits._
import com.sos.scheduler.engine.common.scalautil.xmls.SafeXML
import com.sos.scheduler.engine.common.sprayutils.JsObjectMarshallers._
import com.sos.scheduler.engine.common.time.ScalaTime._
import com.sos.scheduler.engine.common.time.{Stopwatch, WaitForCondition}
import com.sos.scheduler.engine.common.utils.FreeTcpPortFinder.findRandomFreeTcpPort
import com.sos.scheduler.engine.data.event.{Event, EventId}
import com.sos.scheduler.engine.data.events.EventJsonFormat
import com.sos.scheduler.engine.data.filebased.FileBasedState
import com.sos.scheduler.engine.data.job.{JobPath, TaskId}
import com.sos.scheduler.engine.data.jobchain.{EndNodeOverview, JobChainDetails, JobChainOverview, NodeId, NodeKey, SimpleJobNodeOverview}
import com.sos.scheduler.engine.data.order.{OrderKey, OrderStepStarted}
import com.sos.scheduler.engine.data.queries.{JobChainQuery, OrderQuery, PathQuery}
import com.sos.scheduler.engine.data.scheduler.{SchedulerId, SchedulerState}
import com.sos.scheduler.engine.data.xmlcommands.{ModifyOrderCommand, OrderCommand}
import com.sos.scheduler.engine.kernel.DirectSchedulerClient
import com.sos.scheduler.engine.kernel.job.TaskSubsystemClient
import com.sos.scheduler.engine.kernel.variable.SchedulerVariableSet
import com.sos.scheduler.engine.test.EventBusTestFutures.implicits.RichEventBus
import com.sos.scheduler.engine.test.configuration.TestConfiguration
import com.sos.scheduler.engine.test.scalatest.ScalaSchedulerTest
import com.sos.scheduler.engine.tests.jira.js1642.Data._
import java.nio.file.Files.deleteIfExists
import org.junit.runner.RunWith
import org.scalatest.FreeSpec
import org.scalatest.Matchers._
import org.scalatest.junit.JUnitRunner
import scala.collection.{immutable, mutable}
import scala.concurrent.{ExecutionContext, Future}
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
final class JS1642IT extends FreeSpec with ScalaSchedulerTest with SpeedTests {

  private lazy val httpPort = findRandomFreeTcpPort()
  private lazy val schedulerUri = s"http://127.0.0.1:$httpPort"
  protected lazy val directSchedulerClient = instance[DirectSchedulerClient]
  protected lazy val webSchedulerClient = new StandardWebSchedulerClient(schedulerUri).closeWithCloser
  protected override lazy val testConfiguration = TestConfiguration(getClass,
    //binariesDebugMode = Some(com.sos.scheduler.engine.test.binary.CppBinariesDebugMode.release),
    mainArguments = List(s"-http-port=$httpPort", "-distributed-orders"))
  private implicit lazy val executionContext = instance[ExecutionContext]
  private lazy val taskSubsystem = controller.instance[TaskSubsystemClient]
  private val orderKeyToTaskId = mutable.Map[OrderKey, TaskId]()

  private object barrier {
    lazy val file = testEnvironment.tmpDirectory / "TEST-BARRIER"

    def touchFile() = {
      val variableSet = instance[SchedulerVariableSet]
      variableSet(TestJob.BarrierFileVariableName) = file.toString
      touch(file)
      onClose { deleteIfExists(file) }
    }
  }

  private val setting = List[(String, () ⇒ SchedulerClient)](
    "DirectSchedulerClient" → { () ⇒ directSchedulerClient },
    "WebSchedulerClient" → { () ⇒ webSchedulerClient })

  override protected def onSchedulerActivated() = {
    super.onSchedulerActivated()
    eventReader.start()
    barrier.touchFile()
    scheduler executeXml OrderCommand(aAdHocOrderKey, at = Some(OrderStartAt), suspended = Some(true))
    scheduler executeXml OrderCommand(xbAdHocDistributedOrderKey, at = None)
    startOrderProcessing()
  }

  private object eventReader {
    val directEvents = mutable.Buffer[Event]()
    val webEvents = mutable.Buffer[Event]()

    def start(): Unit = {
      eventBus.onHot[Event] {
        case event if EventJsonFormat canSerialize event ⇒ directEvents += event
      }
      start(EventId.BeforeFirst)
    }

    private def start(after: EventId): Unit = {
      for (events ← webSchedulerClient.events(after)) {
        this.webEvents ++= events map { _.event }
        start(after = if (events.isEmpty) after else events.last.eventId)
      }
    }

    def check() = {
      assert(webEvents.nonEmpty)
      WaitForCondition.waitForCondition(5.s, 100.ms) { webEvents.size == directEvents.size }
      assert(webEvents == directEvents)
    }
  }

  private def startOrderProcessing() = {
    val expectedTaskIds = ProcessableOrderKeys.indices map { i ⇒ TaskId.First + i }
    for ((orderKey, expectedTaskId) ← ProcessableOrderKeys zip expectedTaskIds) {
      val event = eventBus.awaitingKeyedEvent[OrderStepStarted](orderKey) {
        scheduler executeXml ModifyOrderCommand(orderKey, suspended = Some(false))
      }
      assert(event.taskId == expectedTaskId)
      orderKeyToTaskId(orderKey) = event.taskId
    }
  }

  lazy val data = new Data(
    taskIdToStartedAt = (for (taskId ← 3 to 5 map TaskId.apply) yield taskId → taskSubsystem.task(taskId).processStartedAt.get).toMap)
  import data._

  for ((testGroup, getClient) ← setting) testGroup - {
    lazy val client = getClient()

    "overview" in {
      val overview = client.overview await TestTimeout
      assert(overview == (directSchedulerClient.overview await TestTimeout))
      assert(overview.schedulerId == SchedulerId("test"))
      assert(overview.state == SchedulerState.running)
    }

    "orderOverviews" in {
      val orders = client.orderOverviews await TestTimeout
      assert(orders == (directSchedulerClient.orderOverviews await TestTimeout))
      assert(orders.toVector.sorted == ExpectedOrderOverviews)
    }

    "orderOverviews speed" in {
      Stopwatch.measureTime(50, s""""orderOverviews with $OrderCount orders"""") {
        client.orderOverviews await TestTimeout
      }
    }

    "ordersComplemented" in {
      val ordersComplemented = client.ordersComplemented await TestTimeout
      assert(ordersComplemented == (directSchedulerClient.ordersComplemented await TestTimeout))
      assert(ordersComplemented == ExpectedOrderOrdersComplemented)
    }

    "orderTreeComplemented" in {
      val treeOverview = client.orderTreeComplementedBy(OrderQuery.All) await TestTimeout
      assert(treeOverview == (directSchedulerClient.orderTreeComplementedBy(OrderQuery.All) await TestTimeout))
      assert(treeOverview == ExpectedOrderTreeComplemented)
    }

    "ordersComplementedBy isSuspended" in {
      val orderQuery = OrderQuery(isSuspended = Some(true))
      val ordersComplemented = client.ordersComplementedBy(orderQuery) await TestTimeout
      assert(ordersComplemented == (directSchedulerClient.ordersComplementedBy(orderQuery) await TestTimeout))
      assert(ordersComplemented == ExpectedOrderOrdersComplemented.copy(
        orders = ExpectedOrderOrdersComplemented.orders filter { _.isSuspended },
        usedTasks = Nil,
        usedJobs = ExpectedOrderOrdersComplemented.usedJobs,
        usedProcessClasses = Nil,
        usedNodes = Vector(
          SimpleJobNodeOverview(NodeKey(aJobChainPath, NodeId("100")), NodeId("END"), NodeId(""), TestJobPath, orderCount = 3),
          SimpleJobNodeOverview(NodeKey(xaJobChainPath, NodeId("100")), NodeId("END"), NodeId(""), XFolderTestJobPath, orderCount = 2))))
    }

    "ordersComplementedBy query /aJobChain" in {
      val query = OrderQuery(jobChainPathQuery = PathQuery("/aJobChain"))
      val ordersComplemented = client.ordersComplementedBy(query) await TestTimeout
      assert(ordersComplemented == (directSchedulerClient.ordersComplementedBy(query) await TestTimeout))
      assert((ordersComplemented.orders map { _.orderKey }).toSet == Set(a1OrderKey, a2OrderKey, aAdHocOrderKey))
    }

    "ordersComplementedBy query /aJobChain/ throws SCHEDULER-161" in {
      val query = OrderQuery(jobChainPathQuery = PathQuery("/aJobChain/"))
      intercept[RuntimeException] {
        client.ordersComplementedBy(query) await TestTimeout
      } .getMessage should include ("SCHEDULER-161")
    }

    "ordersComplementedBy query /xFolder/" in {
      val orderQuery = OrderQuery(jobChainPathQuery = PathQuery("/xFolder/"))
      val ordersComplemented = client.ordersComplementedBy(orderQuery) await TestTimeout
      assert(ordersComplemented == (directSchedulerClient.ordersComplementedBy(orderQuery) await TestTimeout))
      assert((ordersComplemented.orders map { _.orderKey }).toSet == Set(xa1OrderKey, xa2OrderKey, xb1OrderKey, xbAdHocDistributedOrderKey))
    }

    "ordersComplementedBy query /xFolder throws SCHEDULER-161" in {
      val query = OrderQuery(jobChainPathQuery = PathQuery("/xFolder"))
      intercept[RuntimeException] {
        client.ordersComplementedBy(query) await TestTimeout
      } .getMessage should include ("SCHEDULER-161")
    }

    "jobChainOverview All" in {
      assert((client.jobChainOverviewsBy(JobChainQuery.All) await TestTimeout).toSet == Set(
        JobChainOverview(aJobChainPath, FileBasedState.active, isDistributed = false),
        JobChainOverview(bJobChainPath, FileBasedState.active, isDistributed = false),
        JobChainOverview(xaJobChainPath, FileBasedState.active, isDistributed = false),
        JobChainOverview(xbJobChainPath, FileBasedState.active, isDistributed = true)))
    }

    "jobChainOverview query" in {
      val query = JobChainQuery.Standard(PathQuery("/xFolder/"))
      val jobChainOverviews: immutable.Seq[JobChainOverview] = client.jobChainOverviewsBy(query) await TestTimeout
      assert(jobChainOverviews == (directSchedulerClient.jobChainOverviewsBy(query) await TestTimeout))
      assert(jobChainOverviews.toSet == Set(
        JobChainOverview(xaJobChainPath, FileBasedState.active, isDistributed = false),
        JobChainOverview(xbJobChainPath, FileBasedState.active, isDistributed = true)))
    }

    "jobChainDetails" in {
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
              NodeKey(xaJobChainPath, NodeId("100")),
              nextNodeId = NodeId("END"),
              errorNodeId = NodeId(""),
              JobPath("/xFolder/test"),
              orderCount = 2),
            EndNodeOverview(
              NodeKey(xaJobChainPath, NodeId("END"))))))
      assert(jobChainDetails.sourceXml.get startsWith "<job_chain ")
    }

    "ordersComplemented speed" in {
      Stopwatch.measureTime(50, "ordersComplemented") {
        client.ordersComplemented await TestTimeout
      }
    }

    "command" - {
      "<show_state> as xml.Elem" in {
        testValidExecute {
          client.execute(<show_state/>)
        }
        testThrowingExecute {
          client.execute(<UNKNOWN/>)
        }
        testValidExecute {
          client.uncheckedExecute(<show_state/>)
        }
        testUncheckedExecute {
          client.uncheckedExecute(<UNKNOWN/>)
        }
      }

      "<show_state> as String" in {
        testValidExecute {
          client.executeXml("<show_state/>")
        }
        testValidExecute {
          client.uncheckedExecuteXml("<show_state/>")
        }
        testThrowingExecute {
          client.executeXml("<UNKNOWN/>")
        }
        testUncheckedExecute {
          client.uncheckedExecuteXml("<UNKNOWN/>")
        }
      }

      "<show_state> as ByteString" in {
        testValidExecute {
          client.executeXml(ByteString("<show_state/>"))
        }
        testValidExecute {
          client.uncheckedExecuteXml(ByteString("<show_state/>"))
        }
        testThrowingExecute {
          client.executeXml(ByteString("<UNKNOWN/>"))
        }
        testUncheckedExecute {
          client.uncheckedExecuteXml(ByteString("<UNKNOWN/>"))
        }
      }

      def testValidExecute(execute: ⇒ Future[String]): Unit = {
        val response = execute map SafeXML.loadString await TestTimeout
        val state = response \ "answer" \ "state"
        assert((state \ "@state").toString == "running")
        assert((state \ "@ip_address").toString == "127.0.0.1")
      }

      def testThrowingExecute(execute: ⇒ Future[String]): Unit =
        intercept[RuntimeException] {
          controller.toleratingErrorCodes(_ ⇒ true) {
            execute await TestTimeout
          }
        }

      def testUncheckedExecute(execute: ⇒ Future[String]): Unit = {
        val response = controller.toleratingErrorCodes(_ ⇒ true) {
          execute map SafeXML.loadString await TestTimeout
        }
        assert((response \ "answer" \ "ERROR" \ "@code").toString.nonEmpty)
      }

      "Speed test <show_state>" in {
        Stopwatch.measureTime(10, "<show_state what='orders'>") {
          client.executeXml("<show_state what='orders'/>") map SafeXML.loadString await TestTimeout
        }
      }
    }
  }

  "StandardWebSchedulerClient in Java" in {
    SchedulerClientJavaTester.run(schedulerUri)
  }

  "JSON" - {
    "overview" in {
      val overviewString = webSchedulerClient.get[String](_.overview) await TestTimeout
      testRegexJson(
        json = overviewString,
        patternMap = Map(
          "version" → """\d+\..+""".r,
          "version" → """.+""".r,
          "startedAt" → AnyIsoTimestamp,
          "schedulerId" → "test",
          "httpPort" → httpPort,
          "pid" → AnyInt,
          "state" → "running"))
    }

    "orderOverviews" in {
      val orderOverviews = webSchedulerClient.get[JsArray](_.order.overviews()) await TestTimeout
      assert(orderOverviews == ExpectedOrderOverviewsJsArray)
    }

    "ordersComplemented" in {
      val ordersComplemented = webSchedulerClient.get[JsObject](_.order.ordersComplemented()) await TestTimeout
      val orderedOrdersComplemented = JsObject(ordersComplemented.fields ++ Map(
        "orders" → ordersComplemented.fields("orders").asInstanceOf[JsArray],
        "usedTasks" → ordersComplemented.fields("usedTasks").asInstanceOf[JsArray],
        "usedJobs" → ordersComplemented.fields("usedJobs").asInstanceOf[JsArray]))
      assert(orderedOrdersComplemented == ExpectedOrdersOrdersComplementedJsObject)
    }

    "orderTreeComplemented" in {
      val tree = webSchedulerClient.get[JsObject](_.order.treeComplemented) await TestTimeout
      assert(tree == ExpectedOrderTreeComplementedJsObject)
    }
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

    "order.ordersComplemented" in {
      val html = webSchedulerClient.get[String](_.order.ordersComplemented(), accept = `text/html`) await TestTimeout
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
  }

  "Web service error behavior" - {
    "jobscheduler/master/api/ERROR-500" in {
      intercept[UnsuccessfulResponseException] { webSchedulerClient.get[String](_.uriString(s"TEST/ERROR-500")) await TestTimeout }
        .response.status shouldEqual InternalServerError
    }

    "jobscheduler/master/api/UNKNOWN" in {
      intercept[UnsuccessfulResponseException] { webSchedulerClient.get[String](_.uriString("TEST/UNKNOWN")) await TestTimeout }
        .response.status shouldEqual NotFound
    }
  }

  "events" in {
    eventReader.check()
  }

  addOptionalSpeedTests()
}
