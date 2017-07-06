package com.sos.scheduler.engine.tests.jira.js1642

import akka.util.ByteString
import com.google.common.io.Files.touch
import com.sos.scheduler.engine.base.sprayjson.JsonRegexMatcher._
import com.sos.scheduler.engine.base.sprayjson.SprayJson.implicits._
import com.sos.scheduler.engine.base.system.SystemInformation
import com.sos.scheduler.engine.client.api.SchedulerClient
import com.sos.scheduler.engine.client.web.StandardWebSchedulerClient
import com.sos.scheduler.engine.common.event.EventIdGenerator
import com.sos.scheduler.engine.common.event.collector.EventCollector
import com.sos.scheduler.engine.common.scalautil.Closers.implicits._
import com.sos.scheduler.engine.common.scalautil.FileUtils.implicits._
import com.sos.scheduler.engine.common.scalautil.Futures.implicits._
import com.sos.scheduler.engine.common.scalautil.Logger
import com.sos.scheduler.engine.common.scalautil.xmls.SafeXML
import com.sos.scheduler.engine.common.scalautil.xmls.ScalaXmls.implicits.RichXmlFile
import com.sos.scheduler.engine.common.sprayutils.JsObjectMarshallers._
import com.sos.scheduler.engine.common.time.ScalaTime._
import com.sos.scheduler.engine.common.time.WaitForCondition.waitForCondition
import com.sos.scheduler.engine.common.time.{Stopwatch, WaitForCondition}
import com.sos.scheduler.engine.common.utils.FreeTcpPortFinder.findRandomFreeTcpPort
import com.sos.scheduler.engine.common.utils.IntelliJUtils.intelliJuseImports
import com.sos.scheduler.engine.data.compounds.{OrderTreeComplemented, OrdersComplemented}
import com.sos.scheduler.engine.data.event.{EventId, EventRequest, EventSeq, KeyedEvent, Snapshot}
import com.sos.scheduler.engine.data.filebased.{FileBasedAdded, FileBasedDetailed, FileBasedOverview, FileBasedState}
import com.sos.scheduler.engine.data.folder.FolderPath
import com.sos.scheduler.engine.data.job.{JobDescription, JobOverview, JobPath, JobState, TaskId}
import com.sos.scheduler.engine.data.jobchain.{EndNodeOverview, JobChainDetailed, JobChainOverview, JobChainPath, NestedJobChainNodeOverview, NodeId}
import com.sos.scheduler.engine.data.order.{JocOrderStatistics, JocOrderStatisticsChanged, OrderDetailed, OrderKey, OrderOverview, OrderStepStarted}
import com.sos.scheduler.engine.data.processclass.ProcessClassDetailed
import com.sos.scheduler.engine.data.queries.{JobChainNodeQuery, JobChainQuery, OrderQuery, PathQuery}
import com.sos.scheduler.engine.data.scheduler.{SchedulerId, SchedulerInitiated, SchedulerOverview, SchedulerState}
import com.sos.scheduler.engine.data.system.JavaInformation
import com.sos.scheduler.engine.data.xmlcommands.{ModifyOrderCommand, OrderCommand}
import com.sos.scheduler.engine.kernel.DirectSchedulerClient
import com.sos.scheduler.engine.kernel.async.SchedulerThreadFutures._
import com.sos.scheduler.engine.kernel.cppproxy.SpoolerC
import com.sos.scheduler.engine.kernel.folder.FolderSubsystemClient
import com.sos.scheduler.engine.kernel.job.TaskSubsystemClient
import com.sos.scheduler.engine.kernel.scheduler.SchedulerConstants.{FileOrderSinkJobPath, ServiceForwarderJobPath}
import com.sos.scheduler.engine.kernel.variable.SchedulerVariableSet
import com.sos.scheduler.engine.test.EventBusTestFutures.implicits.RichEventBus
import com.sos.scheduler.engine.test.SchedulerTestUtils.jobChainOverview
import com.sos.scheduler.engine.test.configuration.{HostwareDatabaseConfiguration, InMemoryDatabaseConfiguration, TestConfiguration}
import com.sos.scheduler.engine.test.scalatest.ScalaSchedulerTest
import com.sos.scheduler.engine.tests.jira.js1642.Data._
import com.sos.scheduler.engine.tests.jira.js1642.JS1642IT._
import java.nio.file.Files.deleteIfExists
import org.junit.runner.RunWith
import org.scalatest.FreeSpec
import org.scalatest.Matchers._
import org.scalatest.junit.JUnitRunner
import scala.collection.{immutable, mutable}
import scala.concurrent.{ExecutionContext, Future}
import spray.http.MediaTypes.{`text/html`, `text/richtext`}
import spray.http.StatusCodes.{BadRequest, InternalServerError, NotAcceptable, NotFound}
import spray.httpx.UnsuccessfulResponseException
import spray.httpx.unmarshalling.PimpedHttpResponse
import spray.json._

/**
  * JS-1642 WebSchedulerClient and NewWebServicePlugin.
  *
  * @author Joacim Zschimmer
  */
@RunWith(classOf[JUnitRunner])
final class JS1642IT extends FreeSpec with ScalaSchedulerTest with SpeedTests {

  private lazy val httpPort = findRandomFreeTcpPort()
  protected lazy val directSchedulerClient = instance[DirectSchedulerClient]
  protected lazy val webSchedulerClient = new StandardWebSchedulerClient(s"http://127.0.0.1:$httpPort").closeWithCloser
  protected override lazy val testConfiguration = TestConfiguration(getClass,
    mainArguments = List(s"-http-port=127.0.0.1:$httpPort", "-distributed-orders", "-suppress-watchdog-thread"),
    database = Some(
      if (sys.props contains "test.mysql")
        HostwareDatabaseConfiguration("jdbc -class=com.mysql.jdbc.Driver -user=jobscheduler -password=jobscheduler jdbc:mysql://127.0.0.1/jobscheduler")
      else
        InMemoryDatabaseConfiguration))
  private implicit lazy val executionContext = instance[ExecutionContext]
  private lazy val taskSubsystem = instance[TaskSubsystemClient]
  private val orderKeyToTaskId = mutable.Map[OrderKey, TaskId]()

  private object barrier {
    private lazy val file = testEnvironment.tmpDirectory / "TEST-BARRIER"

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

  private lazy val eventReader = new EventReader(webSchedulerClient, instance[EventIdGenerator], controller).closeWithCloser

  private lazy val data = new Data(
    taskIdToStartedAt = (for (taskId ← 3 to 5 map TaskId.apply) yield taskId → taskSubsystem.task(taskId).processStartedAt.get).toMap)
  import data._

  override protected def checkedBeforeAll() = {
    eventBus.onHot[SchedulerInitiated.type] {
      case _ ⇒ instance[EventCollector]  // Start collection of events before Scheduler, to have the very first events available.
    }
    super.checkedBeforeAll()
  }

  override protected def onSchedulerActivated() = {
    super.onSchedulerActivated()
    eventReader.start()
    barrier.touchFile()
    scheduler executeXml OrderCommand(aAdHocOrderKey, at = Some(OrderStartAt), suspended = Some(true))
    scheduler executeXml OrderCommand(xbAdHocDistributedOrderKey, at = None)
    startOrderProcessing()
    testEnvironment.fileFromPath(b1OrderKey).append(" ")
    instance[FolderSubsystemClient].updateFolders()   // Replacement is pending
  }

  private def startOrderProcessing() = {
    val expectedTaskIds = ProcessableOrderKeys.indices map { i ⇒ TaskId.First + i }
    for ((orderKey, expectedTaskId) ← ProcessableOrderKeys zip expectedTaskIds) {
      val event = eventBus.awaiting[OrderStepStarted](orderKey) {
        scheduler executeXml ModifyOrderCommand(orderKey, suspended = Some(false))
      }
      assert(event.taskId == expectedTaskId)
      orderKeyToTaskId(orderKey) = event.taskId
    }
  }

  "http_url" in {
    assert(inSchedulerThread { instance[SpoolerC].http_url } == s"http://127.0.0.1:$httpPort")
  }

  "show_state cluster_member" in {
    val response = scheduler executeXml <show_state what="cluster"/>
    assert((response.elem \ "answer" \ "state" \ "cluster" \ "cluster_member" \ "@http_url").toString ==
      s"http://127.0.0.1:$httpPort")
  }

  "overview" in {
    val overview = fetchWebAndDirectEqualized[SchedulerOverview](
      _.overview,
      _.copy(system = SystemInformation.ForTest, java = JavaInformation.ForTest))
    assert(overview.schedulerId == SchedulerId("test"))
    assert(overview.state == SchedulerState.running)
  }

  "anyTypeFileBaseds FileBasedOverview" in {
    val fileBasedOverviews = fetchWebAndDirect[immutable.Seq[FileBasedOverview]](_.anyTypeFileBaseds[FileBasedOverview](xFolderPath))
    assert(fileBasedOverviews.toSet == Set(
      FileBasedOverview(FolderPath("/xFolder"), FileBasedState.active),
      FileBasedOverview(JobPath("/xFolder/test-b"), FileBasedState.incomplete),
      FileBasedOverview(JobChainPath("/xFolder/x-bJobChain"), FileBasedState.active),
      FileBasedOverview(JobChainPath("/xFolder/x-aJobChain"), FileBasedState.active),
      FileBasedOverview(OrderKey("/xFolder/x-aJobChain", "2"), FileBasedState.active),
      FileBasedOverview(OrderKey("/xFolder/x-aJobChain", "1"), FileBasedState.active),
      FileBasedOverview(OrderKey("/xFolder/x-bJobChain", "1"), FileBasedState.active)))
  }

  "eventsByPath" in {
    val request = EventRequest.singleClass[FileBasedAdded.type](after = EventId.BeforeFirst, timeout = 0.s)
    val query = PathQuery(xFolderPath)
    fetchWebAndDirect[EventSeq[immutable.Seq, KeyedEvent[FileBasedAdded.type]]](_.eventsByPath[FileBasedAdded.type](request, query)) match {
      case nonEmpty: EventSeq.NonEmpty[immutable.Seq, KeyedEvent[FileBasedAdded.type]] ⇒
        assertResult(Set(
          KeyedEvent(FileBasedAdded)(JobChainPath("/xFolder/x-aJobChain")),
          KeyedEvent(FileBasedAdded)(OrderKey("/xFolder/x-aJobChain", "1")),
          KeyedEvent(FileBasedAdded)(OrderKey("/xFolder/x-aJobChain", "2")),
          KeyedEvent(FileBasedAdded)(JobChainPath("/xFolder/x-bJobChain")),
          KeyedEvent(FileBasedAdded)(OrderKey("/xFolder/x-bJobChain", "1")),
          KeyedEvent(FileBasedAdded)(JobPath("/xFolder/test-b"))))
        {
          nonEmpty.eventSnapshots
            .map { _.value }
            .filterNot { _ == KeyedEvent(FileBasedAdded)(FolderPath("/xFolder")) }  // For some reason, this event does not occur always under Linux
            .toSet
        }

      case o ⇒ fail(s"Not expected: $o")
    }
  }

  "order" - {
    "FileBasedDetailed" in {
      val fileBasedDetailed = fetchWebAndDirectEqualized[FileBasedDetailed](
        _.fileBased[OrderKey, FileBasedDetailed](a1OrderKey),
        _.copy(sourceXml = None))
      val file = testEnvironment.fileFromPath(a1OrderKey)
      assert(SafeXML.loadString(fileBasedDetailed.sourceXml.get) == file.xml)
      assert(fileBasedDetailed.fileModifiedAt.isDefined)
    }

    "orders[OrderOverview]" in {
      val orders: immutable.Seq[OrderOverview] = fetchWebAndDirect {
        _.orders[OrderOverview]
      }
      assert((orders.toVector.sorted map normalizeOrderOverview) == ExpectedOrderOverviews.sorted)
    }

    "ordersComplemented" in {
      val ordersComplemented: OrdersComplemented[OrderOverview] = fetchWebAndDirect {
        _.ordersComplemented[OrderOverview]
      }
      assert(ordersComplemented.copy(orders = ordersComplemented.orders map normalizeOrderOverview) ==
        ExpectedOrdersComplemented)
    }

    "orderTreeComplemented" in {
      val treeOverview: OrderTreeComplemented[OrderOverview] = fetchWebAndDirect {
        _.orderTreeComplementedBy[OrderOverview](OrderQuery.All)
      }
      assert(treeOverview.copy(orderTree = treeOverview.orderTree mapLeafs normalizeOrderOverview) ==
        ExpectedOrderTreeComplemented)
    }

    "ordersComplementedBy isSuspended" in {
      val orderQuery = OrderQuery(isSuspended = Some(true))
      val ordersComplemented: OrdersComplemented[OrderOverview] = fetchWebAndDirect {
        _.ordersComplementedBy[OrderOverview](orderQuery)
      }
      assert(ordersComplemented == ExpectedSuspendedOrdersComplemented)
    }

    "ordersComplementedBy query /aJobChain" in {
      val query = OrderQuery(JobChainQuery(PathQuery[JobChainPath]("/aJobChain")))
      val ordersComplemented: OrdersComplemented[OrderOverview] = fetchWebAndDirect {
        _.ordersComplementedBy[OrderOverview](query)
      }
      assert((ordersComplemented.orders map { _.orderKey }).toSet == Set(a1OrderKey, a2OrderKey, aAdHocOrderKey))
    }

    "ordersComplementedBy query /aJobChain/ throws SCHEDULER-161" in {
      val query = OrderQuery(JobChainQuery(PathQuery[JobChainPath]("/aJobChain/")))
      intercept[RuntimeException] {
        fetchWebAndDirect {
          _.ordersComplementedBy[OrderOverview](query)
        }
      } .getMessage should include ("SCHEDULER-161")
    }

    "ordersComplementedBy query /xFolder/" in {
      val orderQuery = OrderQuery(JobChainQuery(PathQuery[JobChainPath]("/xFolder/")))
      val ordersComplemented = fetchWebAndDirect[OrdersComplemented[OrderOverview]] {
        _.ordersComplementedBy[OrderOverview](orderQuery)
      }
      assert((ordersComplemented.orders map { _.orderKey }).toSet == Set(xa1OrderKey, xa2OrderKey, xb1OrderKey, xbAdHocDistributedOrderKey))
    }

    "ordersComplementedBy query /xFolder throws SCHEDULER-161" in {
      val query = OrderQuery(JobChainQuery(PathQuery[JobChainPath]("/xFolder")))
      intercept[RuntimeException] {
        fetchWebAndDirect {
          _.ordersComplementedBy[OrderOverview](query)
        }
      } .getMessage should include ("SCHEDULER-161")
    }

    "orders query OrderId" in {
      val query = OrderQuery(orderIds = Some(Set(OneOrderId)))
      val orders: immutable.Seq[OrderOverview] = fetchWebAndDirect {
        _.ordersBy[OrderOverview](query)
      }
      assert((orders map { _.orderKey }).toSet == Set(a1OrderKey, b1OrderKey, xa1OrderKey, xb1OrderKey, nestedOrderKey))
    }

    "orders query OrderKey, OrderDetailed" in {
      val query = OrderQuery.All.withOrderKey(b1OrderKey)
      val orders: immutable.Seq[OrderDetailed] = fetchWebAndDirect {
        _.ordersBy[OrderDetailed](query)
      }
      val expectedStateText = "TestJob"
      waitForCondition(99.s, 100.ms) { orders.map(_.stateText) == Vector(expectedStateText) }  // Wait for job
      assert((orders map { o ⇒ o.copy(overview = o.overview.copy(startedAt = None)) }) == Vector(OrderDetailed(
        overview = b1OrderOverview,
        initialNodeId = Some(NodeId("100")),
        title = "B1-TITLE",
        stateText = expectedStateText,
        variables = Map("TEST-VAR" → "TEST-VALUE"))))
    }

    "ordersComplementedBy query by outer orderKey" in {
      val query = OrderQuery(JobChainQuery(nestedOuterJobChainPath), orderIds = Some(Set(nestedOrderKey.id)))
      val orders: immutable.Seq[OrderOverview] = fetchWebAndDirect {
        _.ordersBy[OrderOverview](query)
      }
      assert((orders map { _.orderKey }).toSet == Set(nestedOrderKey))
    }

    "orders single non-existent, non-distributed OrderKey throws exception" in {
      assert(!jobChainOverview(aJobChainPath).isDistributed)
      checkUnknownOrderKeyException(aJobChainPath orderKey "UNKNOWN")
    }

    "orders single non-existent, distributed OrderKey throws exception" in {
      assert(jobChainOverview(xbJobChainPath).isDistributed)
      checkUnknownOrderKeyException(xbJobChainPath orderKey "UNKNOWN")
    }

    "orders query JobPath" in {
      val orderQuery = OrderQuery(JobChainNodeQuery(jobPaths = Some(Set(TestJobPath))))
      val orders: immutable.Seq[OrderOverview] = fetchWebAndDirect {
        _.ordersBy[OrderOverview](orderQuery)
      }
      assert((orders map { _.orderKey }).toSet == Set(a1OrderKey, a2OrderKey, aAdHocOrderKey, b1OrderKey, nestedOrderKey))
    }

    "orders query JobPath of non-existent job, distributed" in {
      val orderQuery = OrderQuery(JobChainNodeQuery(jobPaths = Some(Set(XTestBJobPath))))
      val orders: immutable.Seq[OrderOverview] = fetchWebAndDirect {
        _.ordersBy[OrderOverview](orderQuery)
      }
      assert((orders map { _.orderKey }).toSet == Set(xb1OrderKey, xbAdHocDistributedOrderKey))
    }

    def checkUnknownOrderKeyException(orderKey: OrderKey): Unit = {
      val orderQuery = OrderQuery().withOrderKey(orderKey)
      intercept[RuntimeException] {
        fetchWebAndDirect {
          _.ordersBy[OrderOverview](orderQuery)
        }
      } .getMessage should include ("SCHEDULER-161")
    }

    "getOrdersComplementedBy with GET" - {
      "getOrdersComplementedBy isSuspended" in {
        val orderQuery = OrderQuery(isSuspended = Some(true))
        val ordersComplemented = awaitContent(webSchedulerClient.getOrdersComplementedBy[OrderOverview](orderQuery))
        assert(ordersComplemented == awaitContent(directSchedulerClient.ordersComplementedBy[OrderOverview](orderQuery)))
        assert(ordersComplemented == ExpectedSuspendedOrdersComplemented)
      }
    }

    "jobscheduler/master/api/order/MISSING-JOB-CHAIN,1" in {
      val e = intercept[UnsuccessfulResponseException] {
        webSchedulerClient.get[String](_.uriString("api/order/MISSING-JOB-CHAIN,1")) await TestTimeout
      }
      assert(e.response.status == BadRequest)
      assert(e.response.as[String] == Right("SCHEDULER-161 There is no JobChain '/MISSING-JOB-CHAIN'"))
    }

    "JSON" - {
      "overview" in {
        val overviewString = webSchedulerClient.get[String](_.overview) await TestTimeout
        testRegexJson(
          json = overviewString,
          patternMap = Map(
            "eventId" → AnyLong,
            "version" → """\d+\..+""".r,
            "version" → """.+""".r,
            "startedAt" → AnyIsoTimestamp,
            "schedulerId" → "test",
            "httpPort" → s"127.0.0.1:$httpPort",
            "pid" → AnyInt,
            "state" → "running",
            "system" → AnyRef,
            "java" → AnyRef))
      }

      "orderOverviews" in {
        val snapshot = webSchedulerClient.get[JsObject](_.order[OrderOverview]) await TestTimeout
        assert((snapshot("orders").asJsArray map normalizeOrderOverviewJson) == ExpectedOrderOverviewsJsArray)
      }

      "ordersComplemented" in {
        val ordersComplemented = webSchedulerClient.get[JsObject](_.order.complemented[OrderOverview]()) await TestTimeout
        val orderedOrdersComplemented = JsObject((ordersComplemented.fields - Snapshot.EventIdJsonName) ++ Map(
          "orders" → (ordersComplemented("orders").asJsArray map normalizeOrderOverviewJson),
          "usedTasks" → ordersComplemented("usedTasks").asJsArray,
          "usedJobs" → ordersComplemented("usedJobs").asJsArray))
        assert(orderedOrdersComplemented == ExpectedOrdersOrdersComplementedJsObject)
      }

      "orderTreeComplemented" in {
        val tree = webSchedulerClient.get[JsObject](_.order.treeComplemented[OrderOverview]) await TestTimeout
        val normalized = JsObject(tree.fields - Snapshot.EventIdJsonName) deepMapJsObjects normalizeOrderOverviewJson
        assert(normalized == ExpectedOrderTreeComplementedJsObject)
      }
    }
  }

  "jocOrderStatistics" - {
    def testAllJocOrderStatisticsFuture(client: SchedulerClient): Future[JocOrderStatistics] =
      for (Snapshot(_, orderStatistics: JocOrderStatistics) ← client.jocOrderStatistics(JobChainQuery.All)) yield {
        assert(orderStatistics == JocOrderStatistics(
          total = 9,
          notPlanned = 0,
          planned = 0,
          due = 3,
          started = 3,
          inTask = 3,
          inTaskProcess = 3,
          occupiedByClusterMember = 0,
          setback = 0,
          waitingForResource = 0,
          suspended = 3,
          blacklisted = 0,
          permanent = 7,
          fileOrder = 0))
        orderStatistics
      }

    "/" in {
      assert(testAllJocOrderStatisticsFuture(directSchedulerClient).await(TestTimeout) ==
        testAllJocOrderStatisticsFuture(webSchedulerClient).await(TestTimeout))
    }

    val parallelFactor = 1000
    s"$parallelFactor simultaneously requests" in {
      val stopwatch = new Stopwatch
      (for (_ ← 1 to parallelFactor) yield testAllJocOrderStatisticsFuture(webSchedulerClient)) await TestTimeout
      logger.info(stopwatch.itemsPerSecondString(parallelFactor, "testAllOrderStatistics", "testAllOrderStatistics"))
    }

    s"$xFolderPath" in {
      val orderStatistics: JocOrderStatistics = fetchWebAndDirect {
        _.jocOrderStatistics(JobChainQuery(PathQuery(xFolderPath)))
      }
      assert(orderStatistics == JocOrderStatistics(
        total = 4,
        notPlanned = 0,
        planned = 0,
        due = 3,
        started = 0,
        inTask = 0,
        inTaskProcess = 0,
        occupiedByClusterMember = 0,
        setback = 0,
        waitingForResource = 0,
        suspended = 1,
        blacklisted = 0,
        permanent = 3,
        fileOrder = 0))
    }

    s"NodeId 100" in {
      val orderStatistics: JocOrderStatistics = fetchWebAndDirect {
        _.jocOrderStatistics(JobChainNodeQuery(nodeIds = Some(Set(NodeId("100")))))
      }
      assert(orderStatistics == JocOrderStatistics(
        total = 8,
        notPlanned = 0,
        planned = 0,
        due = 3,
        started = 3,
        inTask = 3,
        inTaskProcess = 3,
        occupiedByClusterMember = 0,
        setback = 0,
        waitingForResource = 0,
        suspended = 2,
        blacklisted = 0,
        permanent = 6,
        fileOrder = 0))
    }

    s"NodeId 200" in {
      val orderStatistics: JocOrderStatistics = fetchWebAndDirect {
        _.jocOrderStatistics(JobChainNodeQuery(nodeIds = Some(Set(NodeId("200")))))
      }
      assert(orderStatistics == JocOrderStatistics.Zero)
    }

    s"Job /test" in {
      val orderStatistics: JocOrderStatistics = fetchWebAndDirect {
        _.jocOrderStatistics(JobChainNodeQuery(jobPaths = Some(Set(TestJobPath))))
      }
      assert(orderStatistics == JocOrderStatistics(
        total = 5,
        notPlanned = 0,
        planned = 0,
        due = 0,
        started = 3,
        inTask = 3,
        inTaskProcess = 3,
        occupiedByClusterMember = 0,
        setback = 0,
        waitingForResource = 0,
        suspended = 2,
        blacklisted = 0,
        permanent = 4,
        fileOrder = 0))
    }

    s"Job /xFolder/test-b, distributed" in {
      val orderStatistics: JocOrderStatistics = fetchWebAndDirect {
        _.jocOrderStatistics(JobChainNodeQuery(jobPaths = Some(Set(XTestBJobPath))))
      }
      assert(orderStatistics == JocOrderStatistics(
        total = 2,
        notPlanned = 0,
        planned = 0,
        due = 2,
        started = 0,
        inTask = 0,
        inTaskProcess = 0,
        occupiedByClusterMember = 0,
        setback = 0,
        waitingForResource = 0,
        suspended = 0,
        blacklisted = 0,
        permanent = 1,
        fileOrder = 0))
    }
  }

  "jobChain" - {
    "jobChainOverview" - {
      "JobChainOverview All" in {
        val jobChainOverviews: immutable.Seq[JobChainOverview] = fetchWebAndDirect {
          _.jobChainOverviewsBy(JobChainQuery.All)
        }
        assert(jobChainOverviews.toSet == Set(
          JobChainOverview(aJobChainPath, FileBasedState.active),
          JobChainOverview(bJobChainPath, FileBasedState.active),
          nestedOuterJobChainOverview,
          nestedInnerJobChainOverview,
          xaJobChainOverview,
          xbJobChainOverview))
      }

      "JobChainOverview query" in {
        val query = JobChainQuery(PathQuery[JobChainPath]("/xFolder/"))
        val jobChainOverviews: immutable.Seq[JobChainOverview] = fetchWebAndDirect {
          _.jobChainOverviewsBy(query)
        }
        assert(jobChainOverviews.toSet == Set(
          xaJobChainOverview,
          xbJobChainOverview))
      }
    }

    "JobChainDetailed" in {
      val jobChainDetailed: JobChainDetailed = fetchWebAndDirect {
        _.jobChainDetailed(xaJobChainPath)
      }
      assert(jobChainDetailed ==
        JobChainDetailed(
          xaJobChainOverview,
          List(
            Xa100NodeOverview,
            EndNodeOverview(
              xaJobChainPath,
              NodeId("END")))))
    }
  }

  "Nested JobChain" - {
    "JobChainDetailed for outer JobChain" in {
      val jobChainDetailed: JobChainDetailed = fetchWebAndDirect {
        _.jobChainDetailed(nestedOuterJobChainPath)
      }
      assert(jobChainDetailed ==
        JobChainDetailed(
          nestedOuterJobChainOverview,
          List(
            NestedJobChainNodeOverview(nestedOuterJobChainPath, NodeId("OUTER-1"), NodeId("END"), NodeId(""), nestedInnerJobChainPath),
            EndNodeOverview(nestedOuterJobChainPath, NodeId("END")))))
    }

    "JobChainDetailed for inner JobChain" in {
      val jobChainDetailed: JobChainDetailed = fetchWebAndDirect {
        _.jobChainDetailed(nestedInnerJobChainPath)
      }
      assert(jobChainDetailed ==
        JobChainDetailed(
          nestedInnerJobChainOverview,
          List(
            NestedInner100NodeOverview,
            EndNodeOverview(nestedInnerJobChainPath, NodeId("END")))))
    }

    for (jobChainPath ← Array(nestedOuterJobChainPath, nestedInnerJobChainPath)) s"Order in $jobChainPath" in {
      val ordersComplemented = fetchWebAndDirect {
        _.ordersComplementedBy[OrderOverview](OrderQuery(JobChainQuery(jobChainPath)))
      }
      assert(ordersComplemented.orders == List(nestedOrderOverview))
      assert(ordersComplemented.usedJobChains.toSet == Set(nestedOuterJobChainOverview, nestedInnerJobChainOverview))
      assert(ordersComplemented.usedJobs == List(TestJobOverview))
    }
  }

  "job" - {
    "JobOverview" - {
      "All" in {
        val jobOverviews: immutable.Seq[JobOverview] = fetchWebAndDirect {
          _.jobs[JobOverview](PathQuery.All)
        }
        assert(jobOverviews.toSet == Set(
          TestJobOverview,
          XTestBJobOverview,
          JobOverview(FileOrderSinkJobPath, FileBasedState.active, defaultProcessClassPath = None,
              JobState.pending, isInPeriod = true, taskLimit = 1, usedTaskCount = 0, obstacles = Set()),
          JobOverview(ServiceForwarderJobPath, FileBasedState.active, defaultProcessClassPath = None,
              JobState.pending, isInPeriod = true, taskLimit = 1, usedTaskCount = 0, obstacles = Set())))
      }

      "Single" in {
        val jobOverview: JobOverview = fetchWebAndDirect {
          _.job[JobOverview](XTestBJobPath)
        }
        assert(jobOverview == XTestBJobOverview)
      }
    }

    "JobDescription" in {
      val jobDescription: JobDescription = fetchWebAndDirect {
        _.job[JobDescription](XTestBJobPath)
      }
      assert(jobDescription == JobDescription(XTestBJobPath, "DESCRIPTION OF /xFolder/test-b"))
    }
  }

  "processClass" - {
    "processClassOverview" - {
      "ProcessClassOverview All" in {
        val processClassDetaileds: immutable.Seq[ProcessClassDetailed] = fetchWebAndDirect {
          _.processClasses[ProcessClassDetailed](PathQuery.All)
        }
        val processClassOverviews = processClassDetaileds map { _.overview }
        assert(processClassOverviews.toSet == Set(DefaultProcessClassOverview, TestProcessClassOverview))
      }
    }

    "ProcessClassDetailed" in {
      val processClassDetailed: ProcessClassDetailed = fetchWebAndDirect {
        _.processClass[ProcessClassDetailed](TestProcessClassPath)
      }
      assert(processClassDetailed.copy(processes = Nil) == TestProcessClassDetailed)
    }
  }

  private def fetchWebAndDirect[A](body: SchedulerClient ⇒ Future[Snapshot[A]]): A =
    fetchWebAndDirectEqualized[A](body, identity)

  private def fetchWebAndDirectEqualized[A](body: SchedulerClient ⇒ Future[Snapshot[A]], equalize: A ⇒ A): A = {
    val a: A = awaitContent(body(webSchedulerClient))
    assert(equalize(a) == equalize(awaitContent(body(directSchedulerClient))))
    a
  }

  "XML commands" - {
    for ((testGroup, getClient) ← setting) testGroup - {
      lazy val client = getClient()

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
      val html = webSchedulerClient.get[String](_.order.complemented[OrderOverview](), accept = `text/html`) await TestTimeout
      assert(html startsWith "<!DOCTYPE html")
      assert(html endsWith "</html>")
      assert(html contains "JobScheduler")
    }
  }

  "WebSchedulerClient.getByUri" in {
    val jsObject = webSchedulerClient.getByUri[JsObject]("api") await TestTimeout
    val Snapshot(_, directOverview) = directSchedulerClient.overview await TestTimeout
    assert(jsObject.fields("version") == JsString(directOverview.version))
  }

  "Error behavior" - {
    "jobscheduler/master/api/ERROR-500" in {
      val e = intercept[UnsuccessfulResponseException] {
        webSchedulerClient.get[String](_.uriString(s"TEST/ERROR-500")) await TestTimeout
      }
      assert(e.response.status == InternalServerError)
    }

    "jobscheduler/master/api/UNKNOWN" in {
      val e = intercept[UnsuccessfulResponseException] {
        webSchedulerClient.get[String](_.uriString("TEST/UNKNOWN")) await TestTimeout
      }
      assert(e.response.status == NotFound)
    }
  }

  "Events" - {
    "JocOrderStatisticsChanged" in {
      val Snapshot(aResponseEventId, EventSeq.NonEmpty(aKeyedEventSnapshots)) =
        webSchedulerClient.orderEvents[JocOrderStatisticsChanged](OrderQuery.All, after = EventId.BeforeFirst, TestTimeout) await TestTimeout
      assert(aResponseEventId >= aKeyedEventSnapshots.last.eventId)
      val aStatistics = aKeyedEventSnapshots.head.value.event.orderStatistics

      val bFuture = webSchedulerClient.orderEvents[JocOrderStatisticsChanged](OrderQuery.All, after = aKeyedEventSnapshots.last.eventId, TestTimeout)
      scheduler executeXml ModifyOrderCommand(aAdHocOrderKey, suspended = Some(false))
      val Snapshot(bResponseEventId, EventSeq.NonEmpty(bKeyedEventSnapshots)) = bFuture await TestTimeout
      assert(bResponseEventId >= bKeyedEventSnapshots.last.eventId)
      val bStatistics = bKeyedEventSnapshots.head.value.event.orderStatistics
      assert(bStatistics == aStatistics.copy(suspended = aStatistics.suspended - 1, planned = aStatistics.planned + 1))

      val cFuture = webSchedulerClient.orderEvents[JocOrderStatisticsChanged](OrderQuery.All, after = bKeyedEventSnapshots.last.eventId, TestTimeout)
      scheduler executeXml ModifyOrderCommand(aAdHocOrderKey, suspended = Some(true))
      val Snapshot(cResponseEventId, EventSeq.NonEmpty(cKeyedEventSnapshots)) = cFuture await TestTimeout
      assert(cResponseEventId >= cKeyedEventSnapshots.last.eventId)
      val cStatistics = cKeyedEventSnapshots.head.value.event.orderStatistics
      assert(cStatistics == aStatistics)
    }

    "events" in {
      eventReader.check()
    }
  }

  "Speed tests" - {
    addOptionalSpeedTests()
  }

  private def awaitContent[A](future: Future[Snapshot[A]]): A =
    (future await TestTimeout).value
}

object JS1642IT {
  intelliJuseImports(JsObjectMarshaller)
  private val logger = Logger(getClass)

  private def normalizeOrderOverview(o: OrderOverview) = o.copy(startedAt = None)

  private def normalizeOrderOverviewJson(o: JsValue) = JsObject(o.asJsObject.fields - "startedAt")
}
