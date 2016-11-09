package com.sos.scheduler.engine.client.web

import akka.util.Timeout
import com.sos.scheduler.engine.base.generic.SecretString
import com.sos.scheduler.engine.base.utils.ScalaUtils.implicitClass
import com.sos.scheduler.engine.client.api.SchedulerClient
import com.sos.scheduler.engine.common.auth.{UserAndPassword, UserId}
import com.sos.scheduler.engine.common.time.ScalaTime._
import com.sos.scheduler.engine.data.agent.AgentAddress
import com.sos.scheduler.engine.data.compounds.{OrderTreeComplemented, OrdersComplemented}
import com.sos.scheduler.engine.data.event.{Event, EventId, EventSeq, KeyedEvent, Snapshot}
import com.sos.scheduler.engine.data.events.schedulerKeyedEventJsonFormat
import com.sos.scheduler.engine.data.filebased.{FileBasedDetailed, TypedPath}
import com.sos.scheduler.engine.data.job.{JobPath, JobView}
import com.sos.scheduler.engine.data.jobchain.{JobChainDetailed, JobChainOverview, JobChainPath}
import com.sos.scheduler.engine.data.order.{OrderKey, OrderStatistics, OrderView, Orders}
import com.sos.scheduler.engine.data.processclass.{ProcessClassPath, ProcessClassView}
import com.sos.scheduler.engine.data.queries.{JobChainNodeQuery, JobChainQuery, OrderQuery, PathQuery}
import com.sos.scheduler.engine.data.scheduler.SchedulerOverview
import java.time.Duration
import scala.collection.immutable
import scala.collection.immutable.Seq
import scala.concurrent.Future
import scala.reflect.ClassTag
import spray.client.pipelining._
import spray.http.CacheDirectives.{`no-cache`, `no-store`}
import spray.http.HttpHeaders.{Accept, `Cache-Control`}
import spray.http.MediaTypes._
import spray.http._
import spray.httpx.SprayJsonSupport._
import spray.httpx.encoding.Gzip
import spray.httpx.marshalling.Marshaller
import spray.httpx.unmarshalling._
import spray.json.DefaultJsonProtocol._

/**
 * Client for JobScheduler Agent.
 * The HTTP requests are considered to be responded within `RequestTimeout`.
 *
 * @author Joacim Zschimmer
 */
trait WebSchedulerClient extends SchedulerClient with WebCommandClient {

  protected def credentials: Option[UserAndPassword] = None

  import actorRefFactory.dispatcher

  private implicit val timeout = Timeout(3600.s.toFiniteDuration)

  protected final def commandUri = uris.command

  def uris: SchedulerUris

  private lazy val httpResponsePipeline = {
    val credentialsHeader: RequestTransformer = credentials match {
      case Some(UserAndPassword(UserId(user), SecretString(pass))) ⇒ addCredentials(BasicHttpCredentials(user, pass))
      case None ⇒ identity
    }
    credentialsHeader ~>
      encode(Gzip) ~>
      sendReceive ~>
      decode(Gzip)
  }

  private lazy val jsonHttpResponsePipeline =
    addHeader(Accept(`application/json`)) ~>
      httpResponsePipeline

  final def overview =
    get[Snapshot[SchedulerOverview]](_.overview)

  final def fileBasedDetailed[P <: TypedPath](path: P): Future[Snapshot[FileBasedDetailed]] =
    for (snapshot ← get[Snapshot[FileBasedDetailed]](_.fileBased(path, returnType = "FileBasedDetailed"))) yield
      for (fileBasedDetailed ← snapshot) yield
        fileBasedDetailed.asTyped[P](path.companion.asInstanceOf[TypedPath.Companion[P]]) // Correct TypedPath (instead of UnknownPath)

  def fileBasedDetaileds[P <: TypedPath: TypedPath.Companion](query: PathQuery): Future[Snapshot[immutable.Seq[FileBasedDetailed]]] =
    for (snapshot ← get[Snapshot[immutable.Seq[FileBasedDetailed]]](_.fileBaseds(query, returnType = "FileBasedDetailed"))) yield
      for (seq ← snapshot) yield
        for (fileBasedDetailed ← seq) yield
          fileBasedDetailed.asTyped[P](implicitly[TypedPath.Companion[P]]) // Correct TypedPath (instead of UnknownPath)

  // Order

  final def order[V <: OrderView: OrderView.Companion](orderKey: OrderKey): Future[Snapshot[V]] =
    get[Snapshot[V]](_.order[V](orderKey))

  final def ordersBy[V <: OrderView: OrderView.Companion](query: OrderQuery): Future[Snapshot[immutable.Seq[V]]] =
    post[OrderQuery, Snapshot[Orders[V]]](_.order.forPost[V], query) map { _ map { _.orders }}

  final def getOrdersBy[V <: OrderView: OrderView.Companion](query: OrderQuery): Future[Snapshot[immutable.Seq[V]]] =
    get[Snapshot[Orders[V]]](_.order[V](query)) map { _ map { _.orders }}

  final def orderTreeComplementedBy[V <: OrderView: OrderView.Companion](query: OrderQuery) =
    post[OrderQuery, Snapshot[OrderTreeComplemented[V]]](_.order.treeComplementedForPost[V], query)

  final def getOrderTreeComplementedBy[V <: OrderView: OrderView.Companion](query: OrderQuery) =
    get[Snapshot[OrderTreeComplemented[V]]](_.order.treeComplemented[V](query))

  final def ordersComplementedBy[V <: OrderView: OrderView.Companion](query: OrderQuery) =
    post[OrderQuery, Snapshot[OrdersComplemented[V]]](_.order.complementedForPost[V], query)

  final def getOrdersComplementedBy[V <: OrderView: OrderView.Companion](query: OrderQuery) =
    get[Snapshot[OrdersComplemented[V]]](_.order.complemented[V](query))

  final def orderStatistics(query: JobChainNodeQuery): Future[Snapshot[OrderStatistics]] =
    post[JobChainNodeQuery, Snapshot[OrderStatistics]](_.order.statisticsForPost(query), query)

  final def orderEvents[E <: Event: ClassTag](query: OrderQuery, after: EventId, timeout: Duration, limit: Int = Int.MaxValue): Future[Snapshot[EventSeq[Seq, KeyedEvent[E]]]] =
    get[Snapshot[EventSeq[Seq, KeyedEvent[E]]]](_.order.events(query, after = after, timeout, limit = limit, returnType = implicitClass[E].getSimpleName))

  // JobChain

  final def jobChainOverview(jobChainPath: JobChainPath) =
    get[Snapshot[JobChainOverview]](_.jobChain.overviews(JobChainQuery(PathQuery(jobChainPath))))

  final def jobChainOverviewsBy(query: JobChainQuery): Future[Snapshot[immutable.Seq[JobChainOverview]]] =
    query.pathQuery match {
      case single: PathQuery.SinglePath ⇒
        for (schedulerResponse ← get[Snapshot[JobChainOverview]](_.jobChain.overview(single.as[JobChainPath]))) yield
          for (o ← schedulerResponse) yield
            Vector(o) // Web service returns a single object (not an array), if path designate a single job chain path
      case _ ⇒
        get[Snapshot[immutable.Seq[JobChainOverview]]](_.jobChain.overviews(query))
    }

  final def jobChainDetailed(jobChainPath: JobChainPath) =
    get[Snapshot[JobChainDetailed]](_.jobChain.detailed(jobChainPath))

  // Job

  def job[V <: JobView: JobView.Companion](path: JobPath) =
    get[Snapshot[V]](_.job[V](path))

  def jobs[V <: JobView: JobView.Companion](query: PathQuery) =
    get[Snapshot[immutable.Seq[V]]](_.job[V](query))

  // ProcessClass

  final def processClass[V <: ProcessClassView: ProcessClassView.Companion](processClassPath: ProcessClassPath) =
    get[Snapshot[V]](_.processClass.view[V](processClassPath))

  final def processClasses[V <: ProcessClassView: ProcessClassView.Companion](query: PathQuery) =
    get[Snapshot[immutable.Seq[V]]](_.processClass.views[V](query))

  // Agent

  final def agentUris: Future[Snapshot[Set[AgentAddress]]] =
    get[Snapshot[Set[AgentAddress]]](_.agent.agentUris)

  final def agentGet[A: FromResponseUnmarshaller](uri: String): Future[A] =
    get[A](_.agent.forward(uri))

  // Event

  final def events[E <: Event: ClassTag](after: EventId, timeout: Duration, limit: Int = Int.MaxValue): Future[Snapshot[EventSeq[Seq, KeyedEvent[E]]]] =
    get[Snapshot[EventSeq[Seq, KeyedEvent[E]]]](_.events(after = after, timeout, limit = limit, returnType = implicitClass[E].getSimpleName))

  final def eventsReverse[E <: Event: ClassTag](after: EventId = EventId.BeforeFirst, limit: Int): Future[Snapshot[immutable.Seq[Snapshot[KeyedEvent[E]]]]] =
    get[Snapshot[immutable.Seq[Snapshot[KeyedEvent[E]]]]](_.eventsReverse(after, limit = limit, returnType = implicitClass[E].getSimpleName))

  // Basic

  final def getByUri[A: FromResponseUnmarshaller](relativeUri: String, accept: MediaType = `application/json`): Future[A] =
    get[A](_.uriString(relativeUri), accept)

  final def get[A: FromResponseUnmarshaller](uri: SchedulerUris ⇒ String, accept: MediaType = `application/json`): Future[A] =
    if (accept == `application/json`)
      jsonUnmarshallingPipeline[A].apply(Get(uri(uris)))
    else
      unmarshallingPipeline[A](accept = accept).apply(Get(uri(uris)))

  private def unmarshallingPipeline[A: FromResponseUnmarshaller](accept: MediaType) =
    addHeader(Accept(accept)) ~> httpResponsePipeline ~> unmarshal[A]

  private final def post[A: Marshaller, B: FromResponseUnmarshaller](uri: SchedulerUris ⇒ String, data: A): Future[B] =
    jsonUnmarshallingPipeline[B].apply(Post(uri(uris), data))

  private def jsonUnmarshallingPipeline[A: FromResponseUnmarshaller] =
    jsonHttpResponsePipeline ~> unmarshal[A]

  override def toString = s"WebSchedulerClient($uris)"
}
