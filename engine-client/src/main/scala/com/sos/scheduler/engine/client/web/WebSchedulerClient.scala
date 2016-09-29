package com.sos.scheduler.engine.client.web

import com.sos.scheduler.engine.base.utils.ScalaUtils.implicitClass
import com.sos.scheduler.engine.client.api.SchedulerClient
import com.sos.scheduler.engine.data.agent.AgentAddress
import com.sos.scheduler.engine.data.compounds.{OrderTreeComplemented, OrdersComplemented}
import com.sos.scheduler.engine.data.event.{Event, EventId, KeyedEvent, Snapshot}
import com.sos.scheduler.engine.data.events.schedulerKeyedEventJsonFormat
import com.sos.scheduler.engine.data.filebased.{FileBasedDetailed, TypedPath}
import com.sos.scheduler.engine.data.jobchain.{JobChainDetailed, JobChainOverview, JobChainPath}
import com.sos.scheduler.engine.data.order.{OrderKey, OrderStatistics, OrderView, Orders}
import com.sos.scheduler.engine.data.queries.{JobChainQuery, OrderQuery, PathQuery}
import com.sos.scheduler.engine.data.scheduler.SchedulerOverview
import scala.collection.immutable
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

  import actorRefFactory.dispatcher

  protected final def commandUri = uris.command

  def uris: SchedulerUris

  private lazy val nonCachingHttpResponsePipeline: HttpRequest ⇒ Future[HttpResponse] =
    addHeader(`Cache-Control`(`no-cache`, `no-store`)) ~>
    encode(Gzip) ~>
    sendReceive ~>
    decode(Gzip)

  private lazy val jsonNonCachingHttpResponsePipeline: HttpRequest ⇒ Future[HttpResponse] =
    addHeader(Accept(`application/json`)) ~>
    nonCachingHttpResponsePipeline

  final def overview =
    get[Snapshot[SchedulerOverview]](_.overview)

  final def fileBasedDetailed[P <: TypedPath: TypedPath.Companion](path: P): Future[Snapshot[FileBasedDetailed]] =
    for (snapshot ← get[Snapshot[FileBasedDetailed]](_.fileBasedDetailed(path))) yield
      for (fileBasedDetailed ← snapshot) yield
        fileBasedDetailed.asTyped[P] // Correct TypedPath (instead of UnknownPath)

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

  final def orderStatistics(query: JobChainQuery): Future[Snapshot[OrderStatistics]] =
    post[JobChainQuery, Snapshot[OrderStatistics]](_.order.statisticsForPost(query), query)

  // JobChain

  final def jobChainOverview(jobChainPath: JobChainPath) =
    get[Snapshot[JobChainOverview]](_.jobChain.overviews(JobChainQuery(PathQuery(jobChainPath))))

  final def jobChainOverviewsBy(query: JobChainQuery): Future[Snapshot[immutable.Seq[JobChainOverview]]] =
    query.pathQuery match {
      case single: PathQuery.SinglePath ⇒
        for (schedulerResponse ← get[Snapshot[JobChainOverview]](_.jobChain.overview(single.as[JobChainPath])))
             yield for (o ← schedulerResponse)
          yield Vector(o) // Web service return a single object (not an array), if path denotes a single job chain path
      case _ ⇒
        get[Snapshot[immutable.Seq[JobChainOverview]]](_.jobChain.overviews(query))
    }

  final def jobChainDetailed(jobChainPath: JobChainPath) =
    get[Snapshot[JobChainDetailed]](_.jobChain.details(jobChainPath))

  // Agent

  final def agentUris: Future[Snapshot[Set[AgentAddress]]] =
    get[Snapshot[Set[AgentAddress]]](_.agent.agentUris)

  final def agentGet[A: FromResponseUnmarshaller](uri: String): Future[A] =
    get[A](_.agent.forward(uri))

  // Event

  final def events[E <: Event: ClassTag](after: EventId, limit: Int = Int.MaxValue, reverse: Boolean = false): Future[Snapshot[immutable.Seq[Snapshot[KeyedEvent[E]]]]] =
    get[Snapshot[immutable.Seq[Snapshot[KeyedEvent[E]]]]](_.events(after = after, limit = limit, reverse = reverse, returnType = implicitClass[E].getSimpleName))

  // Basic

  final def getByUri[A: FromResponseUnmarshaller](relativeUri: String): Future[A] =
    get[A](_.uriString(relativeUri))

  final def get[A: FromResponseUnmarshaller](uri: SchedulerUris ⇒ String): Future[A] =
    jsonUnmarshallingPipeline[A].apply(Get(uri(uris)))

  final def get2[A: FromResponseUnmarshaller](uri: SchedulerUris ⇒ String, accept: MediaType): Future[A] =
    unmarshallingPipeline[A](accept = accept).apply(Get(uri(uris)))

  private def unmarshallingPipeline[A: FromResponseUnmarshaller](accept: MediaType) =
    addHeader(Accept(accept)) ~> nonCachingHttpResponsePipeline ~> unmarshal[A]

  private final def post[A: Marshaller, B: FromResponseUnmarshaller](uri: SchedulerUris ⇒ String, data: A): Future[B] =
    jsonUnmarshallingPipeline[B].apply(Post(uri(uris), data))

  private def jsonUnmarshallingPipeline[A: FromResponseUnmarshaller] =
    jsonNonCachingHttpResponsePipeline ~> unmarshal[A]

  override def toString = s"WebSchedulerClient($uris)"
}
