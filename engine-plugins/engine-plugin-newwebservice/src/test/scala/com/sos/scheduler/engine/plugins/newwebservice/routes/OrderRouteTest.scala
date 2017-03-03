package com.sos.scheduler.engine.plugins.newwebservice.routes

import akka.actor.ActorSystem
import com.sos.jobscheduler.common.event.EventIdGenerator
import com.sos.jobscheduler.common.event.collector.EventCollector
import com.sos.jobscheduler.common.sprayutils.SprayUtils.pathSegments
import com.sos.jobscheduler.common.time.ScalaTime._
import com.sos.jobscheduler.common.time.timer.TimerService
import com.sos.jobscheduler.data.event._
import com.sos.jobscheduler.data.filebased.TypedPath
import com.sos.jobscheduler.data.folder.FolderPath
import com.sos.jobscheduler.data.job.TaskId
import com.sos.scheduler.engine.client.api.{OrderClient, SchedulerOverviewClient}
import com.sos.scheduler.engine.data.compounds.{OrderTreeComplemented, OrdersComplemented}
import com.sos.scheduler.engine.data.events.SchedulerAnyKeyedEventJsonFormat.eventTypedJsonFormat
import com.sos.scheduler.engine.data.filebased.{FileBasedActivated, FileBasedAdded, FileBasedDetailed, FileBasedOverview, FileBasedState}
import com.sos.scheduler.engine.data.job.{JobOverview, JobPath, JobState, TaskOverview, TaskState}
import com.sos.scheduler.engine.data.jobchain.{JobChainOverview, JobChainPath, NodeId, SimpleJobNodeOverview}
import com.sos.scheduler.engine.data.order.{JocOrderStatistics, OrderDetailed, OrderKey, OrderOverview, OrderProcessingState, OrderSourceType, OrderStarted, OrderStepStarted, OrderView, Orders}
import com.sos.scheduler.engine.data.processclass.{ProcessClassOverview, ProcessClassPath}
import com.sos.scheduler.engine.data.queries.{JobChainNodeQuery, OrderQuery}
import com.sos.scheduler.engine.eventbus.SchedulerEventBus
import com.sos.scheduler.engine.kernel.event.DirectEventClient
import com.sos.scheduler.engine.kernel.event.collector.SchedulerEventCollector
import com.sos.scheduler.engine.plugins.newwebservice.html.WebServiceContext
import com.sos.scheduler.engine.plugins.newwebservice.routes.OrderRouteTest._
import java.nio.file.Paths
import java.time.Instant
import java.time.Instant._
import org.junit.runner.RunWith
import org.scalatest.junit.JUnitRunner
import org.scalatest.{BeforeAndAfterAll, FreeSpec}
import scala.collection.immutable.Seq
import scala.concurrent.{ExecutionContext, Future}
import spray.http.HttpHeaders.Accept
import spray.http.MediaTypes.`application/json`
import spray.httpx.SprayJsonSupport._
import spray.json.DefaultJsonProtocol._
import spray.json._
import spray.routing.Route
import spray.testkit.ScalatestRouteTest

/**
  * @author Joacim Zschimmer
  */
@RunWith(classOf[JUnitRunner])
final class OrderRouteTest extends FreeSpec with BeforeAndAfterAll with ScalatestRouteTest with OrderRoute {

  private lazy val actorSystem = ActorSystem("OrderRoute")
  private implicit val eventBus = new SchedulerEventBus
  private implicit lazy val timerService = TimerService(idleTimeout = Some(1.s))

  protected def orderSubsystem = throw new NotImplementedError

  protected def orderStatisticsChangedSource = throw new NotImplementedError

  protected def webServiceContext = new WebServiceContext()

  protected implicit def executionContext = ExecutionContext.global

  protected def schedulerThreadCallQueue = throw new NotImplementedError

  protected def actorRefFactory = actorSystem

  protected val client = new OrderClient with SchedulerOverviewClient with DirectEventClient {
    protected val eventIdGenerator = new EventIdGenerator
    protected val eventCollector = new SchedulerEventCollector(EventCollector.Configuration.ForTest, eventIdGenerator)
    protected def executionContext = OrderRouteTest.this.executionContext

    def fileBasedDetailed[P <: TypedPath](path: P): Future[Stamped[FileBasedDetailed]] =
      respondWith {
        assert(path == A1OrderKey)
        A1FileBasedDetailed
      }

    def order[V <: OrderView: OrderView.Companion](orderKey: OrderKey): Future[Stamped[V]] =
      respondWith(
        implicitly[OrderView.Companion[V]] match {
          case OrderOverview ⇒ A1OrderOverview
          case OrderDetailed ⇒ A1OrderDetailed
        })

    def ordersBy[V <: OrderView: OrderView.Companion](query: OrderQuery) = {
      assert(query == TestOrderQuery)
      respondWith(
        implicitly[OrderView.Companion[V]] match {
          case OrderOverview ⇒ TestOrderOverviews
          case OrderDetailed ⇒ TestOrderDetaileds
        })
    }

    def orderTreeComplementedBy[V <: OrderView: OrderView.Companion](query: OrderQuery): Future[Stamped[OrderTreeComplemented[V]]] = {
      assert(query == TestOrderQuery)
      for (snapshot ← ordersComplementedBy[V](query)) yield
        for (flat ← snapshot) yield
          OrderTreeComplemented.fromOrderComplemented(FolderPath.Root, flat)
    }

    def ordersComplementedBy[V <: OrderView: OrderView.Companion](query: OrderQuery): Future[Stamped[OrdersComplemented[V]]] = {
      assert(query == TestOrderQuery)
      respondWith(
        implicitly[OrderView.Companion[V]] match {
          case OrderOverview ⇒ TestOrdersComplemented
          case OrderDetailed ⇒ DetailedOrdersComplemented
        })
    }

    def jocOrderStatistics(query: JobChainNodeQuery): Future[Stamped[JocOrderStatistics]] = {
      assert(query.matchesAll)
      respondWith(TestOrderStatistics)
    }

    def overview = throw new NotImplementedError

    private def respondWith[A](a: A) = Future.successful(Stamped(TestEventId, a))
  }

  override protected def afterAll() = {
    actorSystem.shutdown()
    timerService.close()
    super.afterAll()
  }

  private def route: Route =
    pathSegments("api/order") {
      orderRoute
    }

  OrderUri in {
    Get(OrderUri) ~> Accept(`application/json`) ~> route ~> check {
      assert(!handled)
    }
  }

  for (uri ← List(
      s"$OrderUri/?return=OrderOverview&isSuspended=false&isOrderSourceType=Permanent")) {
    s"$uri" in {
      Get(uri) ~> Accept(`application/json`) ~> route ~> check {
        val snapshot = responseAs[Stamped[Orders[OrderOverview]]]
        assert(snapshot == Stamped(TestEventId, Orders(TestOrderOverviews)))
      }
    }
  }

  for (uri ← List(
      s"$OrderUri/?return=OrderDetailed&isSuspended=false&isOrderSourceType=Permanent")) {
    s"$uri" in {
      Get(uri) ~> Accept(`application/json`) ~> route ~> check {
        assert(responseAs[Stamped[Orders[OrderDetailed]]] == Stamped(TestEventId, Orders(TestOrderDetaileds)))
      }
    }
  }

  for (uri ← List(
      s"$OrderUri/?isSuspended=false&isOrderSourceType=Permanent",
      s"$OrderUri/?return=OrderTreeComplemented&isSuspended=false&isOrderSourceType=Permanent",
      s"$OrderUri/?return=OrderTreeComplemented/OrderOverview&isSuspended=false&isOrderSourceType=Permanent")) {
    s"$uri" in {
      Get(uri) ~> Accept(`application/json`) ~> route ~> check {
        assert(responseAs[OrderTreeComplemented[OrderOverview]] ==
          OrderTreeComplemented.fromOrderComplemented(FolderPath.Root, TestOrdersComplemented))
      }
    }
  }

  for (uri ← List(
      s"$OrderUri/?return=OrderTreeComplemented/OrderDetailed&isSuspended=false&isOrderSourceType=Permanent")) {
    s"$uri" in {
      Get(uri) ~> Accept(`application/json`) ~> route ~> check {
        assert(responseAs[OrderTreeComplemented[OrderDetailed]] ==
          OrderTreeComplemented.fromOrderComplemented(FolderPath.Root, DetailedOrdersComplemented))
      }
    }
  }

  for (uri ← List(
      s"$OrderUri/?return=OrdersComplemented&isSuspended=false&isOrderSourceType=Permanent",
      s"$OrderUri/?return=OrdersComplemented/OrderOverview&isSuspended=false&isOrderSourceType=Permanent")) {
    s"$uri" in {
      Get(uri) ~> Accept(`application/json`) ~> route ~> check {
        assert(responseAs[OrdersComplemented[OrderOverview]] == TestOrdersComplemented)
      }
    }
  }

  for (uri ← List(
      s"$OrderUri/?return=OrdersComplemented/OrderDetailed&isSuspended=false&isOrderSourceType=Permanent")) {
    s"$uri" in {
      Get(uri) ~> Accept(`application/json`) ~> route ~> check {
        assert(responseAs[OrdersComplemented[OrderDetailed]] == DetailedOrdersComplemented)
      }
    }
  }

  for (uri ← List(
      s"$OrderUri/aJobChain,1?return=OrderOverview&isSuspended=false&isOrderSourceType=Permanent")) {
    s"$uri" in {
      Get(uri) ~> Accept(`application/json`) ~> route ~> check {
        assert(responseAs[Stamped[OrderOverview]].value == A1OrderOverview)
      }
    }
  }

  for (uri ← List(
      s"$OrderUri/aJobChain,1?isSuspended=false&isOrderSourceType=Permanent",
      s"$OrderUri/aJobChain,1?return=OrderDetailed&isSuspended=false&isOrderSourceType=Permanent")) {
    s"$uri" in {
      Get(uri) ~> Accept(`application/json`) ~> route ~> check {
        assert(responseAs[Stamped[OrderDetailed]].value == A1OrderDetailed)
      }
    }
  }

  "POST OrderQuery" in {
    val queryJson = JsObject(
      "isSuspended" → JsFalse,
      "isOrderSourceType" → JsArray(JsString("Permanent")))
    Post("/api/order?return=OrderOverview", queryJson) ~> Accept(`application/json`) ~> route ~> check {
      val snapshot = responseAs[Stamped[Orders[OrderOverview]]]
      assert(snapshot == Stamped(TestEventId, Orders(TestOrderOverviews)))
    }
  }

  for (uri ← List(
      s"$OrderUri/aJobChain,1?return=Event&timeout=60s&after=0")) {
    s"$uri" in {
      for (event ← OrderEvents) eventBus.publish(KeyedEvent(event)(A1OrderKey))
      Get(uri) ~> Accept(`application/json`) ~> route ~> check {
        val EventSeq.NonEmpty(snapshots) = responseAs[TearableEventSeq[Seq, Event]]
        assert((snapshots map { _.value }) == OrderEvents)
      }
    }
  }

  for (uri ← List(
      s"$OrderUri?return=Event&timeout=60s&after=0")) {
    val jobChainPath = A1OrderKey.jobChainPath
    val orderId = A1OrderKey.id
    val queryJson = JsObject(
      "path" → JsString(jobChainPath.string),
      "orderIds" → JsArray(JsString(orderId.string)))
    s"POST $uri" in {
      // Events have been published before.
      Post(uri, queryJson) ~> Accept(`application/json`) ~> route ~> check {
        val EventSeq.NonEmpty(snapshots) = responseAs[TearableEventSeq[Seq, Event]]
        assert((snapshots map { _.value }) == OrderEvents)
      }
    }
  }

  for (uri ← List(
      s"$OrderUri/?return=JocOrderStatistics&timeout=60s")) {
    s"$uri" in {
      Get(uri) ~> Accept(`application/json`) ~> route ~> check {
        assert(responseAs[Stamped[JocOrderStatistics]].value == TestOrderStatistics)
      }
    }
  }
}

object OrderRouteTest {
  private val OrderUri = "/api/order"
  private val AJobChainPath = JobChainPath("/aJobChain")
  private val TestJobPath = JobPath("/test")
  private val A1OrderKey = AJobChainPath orderKey "1"
  private val TestEventId = EventId(Long.MaxValue)

  private val A1FileBasedDetailed =FileBasedDetailed(
    FileBasedOverview(
    A1OrderKey,
    FileBasedState.active),
    Some(Paths.get("/DIR/a,1.order.xml")),
    Some(Instant.parse("2016-09-07T11:22:33.444Z")),
    Some("<?xml version='1.0'?><order/>"))

  private val A1OrderOverview = OrderOverview(
    A1OrderKey,
    FileBasedState.active,
    OrderSourceType.Permanent,
    A1OrderKey.jobChainPath,
    NodeId("100"),
    OrderProcessingState.InTaskProcess(
      TaskId(3),
      ProcessClassPath.Default,
      since = Instant.parse("2016-08-26T11:22:33.444Z"),
      agentUri = None),
    nextStepAt = Some(EPOCH))
  private val A1OrderDetailed = OrderDetailed(A1OrderOverview, priority = 0, title = "")

  private val TestOrderOverviews = Vector(A1OrderOverview)

  private val TestOrderDetaileds = Vector(A1OrderDetailed)

  private val TestOrdersComplemented = OrdersComplemented[OrderOverview](
    TestOrderOverviews,
    Vector(
      JobChainOverview(AJobChainPath, FileBasedState.active)),
    Vector(
      SimpleJobNodeOverview(AJobChainPath, NodeId("100"), NodeId("END"), NodeId(""), TestJobPath, orderCount = 1)),
    Vector(
      JobOverview(TestJobPath, FileBasedState.active, defaultProcessClassPath = None, JobState.running, isInPeriod = true,
        taskLimit = 10, usedTaskCount = 3, obstacles = Set())),
    Vector(
      TaskOverview(TaskId(3), TestJobPath, TaskState.running, Some(ProcessClassPath.Default))),
    Vector(
      ProcessClassOverview(ProcessClassPath.Default, FileBasedState.active, processLimit = 30, usedProcessCount = 3)))

  private val DetailedOrdersComplemented = OrdersComplemented[OrderDetailed](
    TestOrderDetaileds,
    TestOrdersComplemented.usedJobChains,
    TestOrdersComplemented.usedNodes,
    TestOrdersComplemented.usedJobs,
    TestOrdersComplemented.usedTasks,
    TestOrdersComplemented.usedProcessClasses)

  private val TestOrderQuery = OrderQuery(
    isSuspended = Some(false),
    isOrderSourceType = Some(Set(OrderSourceType.Permanent)))

  private val OrderEvents = Vector(
    FileBasedAdded,
    FileBasedActivated,
    OrderStarted,
    OrderStepStarted(NodeId("100"), TaskId(3)))

  private val TestOrderStatistics = {
    JocOrderStatistics(-1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14)
  }
}
