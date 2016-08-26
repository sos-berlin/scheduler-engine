package com.sos.scheduler.engine.plugins.newwebservice.routes

import akka.actor.ActorSystem
import com.sos.scheduler.engine.client.api.{OrderClient, SchedulerOverviewClient}
import com.sos.scheduler.engine.data.compounds.{OrderTreeComplemented, OrdersComplemented}
import com.sos.scheduler.engine.data.event.{EventId, Snapshot}
import com.sos.scheduler.engine.data.filebased.FileBasedState
import com.sos.scheduler.engine.data.folder.FolderPath
import com.sos.scheduler.engine.data.job.{JobOverview, JobPath, JobState, ProcessClassOverview, TaskId, TaskOverview, TaskState}
import com.sos.scheduler.engine.data.jobchain.{JobChainPath, NodeId, NodeKey, SimpleJobNodeOverview}
import com.sos.scheduler.engine.data.order.{OrderDetailed, OrderOverview, OrderProcessingState, OrderSourceType, OrderView}
import com.sos.scheduler.engine.data.processclass.ProcessClassPath
import com.sos.scheduler.engine.data.queries.OrderQuery
import com.sos.scheduler.engine.plugins.newwebservice.html.WebServiceContext
import com.sos.scheduler.engine.plugins.newwebservice.routes.OrderRouteTest._
import java.time.Instant
import java.time.Instant._
import org.junit.runner.RunWith
import org.scalatest.junit.JUnitRunner
import org.scalatest.{BeforeAndAfterAll, FreeSpec}
import scala.collection.immutable
import scala.concurrent.{ExecutionContext, Future}
import spray.http.HttpHeaders.Accept
import spray.http.MediaTypes.`application/json`
import spray.httpx.SprayJsonSupport._
import spray.json.DefaultJsonProtocol._
import spray.routing.Directives._
import spray.routing.Route
import spray.testkit.ScalatestRouteTest

/**
  * @author Joacim Zschimmer
  */
@RunWith(classOf[JUnitRunner])
final class OrderRouteTest extends FreeSpec with BeforeAndAfterAll with ScalatestRouteTest with OrderRoute {

  private lazy val actorSystem = ActorSystem("OrderRoute")

  protected def orderSubsystem = throw new NotImplementedError()

  protected implicit def client = new OrderClient with SchedulerOverviewClient {
    def ordersBy[V <: OrderView: OrderView.Companion](query: OrderQuery) =
      respondWith(
        implicitly[OrderView.Companion[V]] match {
          case OrderOverview ⇒ TestOrderOverviews
          case OrderDetailed ⇒ TestOrderDetaileds
        })


    def orderTreeComplementedBy[V <: OrderView: OrderView.Companion](query: OrderQuery): Future[Snapshot[OrderTreeComplemented[V]]] =
      for (snapshot ← ordersComplementedBy[V](query)) yield
        for (flat ← snapshot) yield
          OrderTreeComplemented.fromOrderComplemented(FolderPath.Root, flat)

    def ordersComplementedBy[V <: OrderView: OrderView.Companion](query: OrderQuery): Future[Snapshot[OrdersComplemented[V]]] =
      respondWith(
        implicitly[OrderView.Companion[V]] match {
          case OrderOverview ⇒ TestOrdersComplemented
          case OrderDetailed ⇒ DetailedOrdersComplemented
        })

    def overview = throw new NotImplementedError

    private def respondWith[A](a: A) = Future.successful(Snapshot(a)(TestEventId))
  }

  protected def webServiceContext = new WebServiceContext()

  protected implicit def executionContext = ExecutionContext.global

  protected def schedulerThreadCallQueue = throw new NotImplementedError

  protected def actorRefFactory = actorSystem

  override protected def afterAll() = {
    actorSystem.shutdown()
    super.afterAll()
  }

  private def route: Route =
    pathPrefix("api" / "order") {
      orderRoute
    }

  "/api/order" in {
    Get("/api/order") ~> Accept(`application/json`) ~> route ~> check {
      assert(!handled)
    }
  }

  for (path ← List(
      "/api/order/?return=OrderOverview")) {
    s"$path" in {
      Get(path) ~> Accept(`application/json`) ~> route ~> check {
        val snapshot = responseAs[Snapshot[Vector[OrderOverview]]]
        assert(snapshot.value == TestOrderOverviews)
        assert(snapshot.eventId == TestEventId)  // Snapshot.equals ignores eventId
        assert(responseAs[Snapshot[Vector[OrderOverview]]] == Snapshot(TestOrderOverviews)(666))
      }
    }
  }

  for (path ← List(
      "/api/order/?return=OrderDetailed")) {
    s"$path" in {
      Get(path) ~> Accept(`application/json`) ~> route ~> check {
        val snapshot = responseAs[Snapshot[Vector[OrderDetailed]]]
        assert(snapshot.value == TestOrderDetaileds)
        assert(snapshot.eventId == TestEventId)  // Snapshot.equals ignores eventId
      }
    }
  }

  for (path ← List(
      "/api/order/",
      "/api/order/?return=OrderTreeComplemented",
      "/api/order/?return=OrderTreeComplemented/OrderOverview")) {
    s"$path" in {
      Get(path) ~> Accept(`application/json`) ~> route ~> check {
        assert(responseAs[OrderTreeComplemented[OrderOverview]] ==
          OrderTreeComplemented.fromOrderComplemented(FolderPath.Root, TestOrdersComplemented))
      }
    }
  }

  for (path ← List(
      "/api/order/?return=OrderTreeComplemented/OrderDetailed")) {
    s"$path" in {
      Get(path) ~> Accept(`application/json`) ~> route ~> check {
        assert(responseAs[OrderTreeComplemented[OrderDetailed]] ==
          OrderTreeComplemented.fromOrderComplemented(FolderPath.Root, DetailedOrdersComplemented))
      }
    }
  }

  for (path ← List(
      "/api/order/?return=OrdersComplemented",
      "/api/order/?return=OrdersComplemented/OrderOverview")) {
    s"$path" in {
      Get(path) ~> Accept(`application/json`) ~> route ~> check {
        assert(responseAs[OrdersComplemented[OrderOverview]] == TestOrdersComplemented)
      }
    }
  }

  for (path ← List(
      "/api/order/?return=OrdersComplemented/OrderDetailed")) {
    s"$path" in {
      Get(path) ~> Accept(`application/json`) ~> route ~> check {
        assert(responseAs[OrdersComplemented[OrderDetailed]] == DetailedOrdersComplemented)
      }
    }
  }
}

object OrderRouteTest {
  private val AJobChainPath = JobChainPath("/aJobChain")
  private val TestJobPath = JobPath("/test")
  private val A1OrderKey = AJobChainPath orderKey "1"
  private val TestEventId = EventId(Long.MaxValue)

  private val A1OrderOverview = OrderOverview(
    A1OrderKey,
    FileBasedState.active,
    OrderSourceType.Permanent,
    NodeId("100"),
    OrderProcessingState.InTaskProcess(
      TaskId(3),
      ProcessClassPath.Default,
      agentUri = None,
      since = Instant.parse("2016-08-26T11:22:33.444Z")),
    nextStepAt = Some(EPOCH))
  private val A1OrderDetailed = OrderDetailed(A1OrderOverview)

  private val TestOrderOverviews = Vector(A1OrderOverview)

  private val TestOrderDetaileds = Vector(A1OrderDetailed)

  private val TestOrdersComplemented = OrdersComplemented[OrderOverview](
    TestOrderOverviews,
    Vector(
      SimpleJobNodeOverview(NodeKey(AJobChainPath, NodeId("100")), NodeId("END"), NodeId(""), TestJobPath, orderCount = 1)),
    Vector(
      JobOverview(TestJobPath, FileBasedState.active, defaultProcessClassPath = None, JobState.running, isInPeriod = true,
        taskLimit = 10, usedTaskCount = 3, obstacles = Set())),
    Vector(
      TaskOverview(TaskId(3), TestJobPath, TaskState.running, ProcessClassPath.Default)),
    Vector(
      ProcessClassOverview(ProcessClassPath.Default, FileBasedState.active, processLimit = 30, usedProcessCount = 3))
  )

  private val DetailedOrdersComplemented = OrdersComplemented[OrderDetailed](
    TestOrderDetaileds,
    TestOrdersComplemented.usedNodes,
    TestOrdersComplemented.usedJobs,
    TestOrdersComplemented.usedTasks,
    TestOrdersComplemented.usedProcessClasses)
}
