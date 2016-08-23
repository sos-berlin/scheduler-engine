package com.sos.scheduler.engine.client.web

import com.sos.scheduler.engine.client.api.SchedulerClient
import com.sos.scheduler.engine.data.compounds.{OrderTreeComplemented, OrdersComplemented}
import com.sos.scheduler.engine.data.event.{AnyKeyedEvent, EventId, Snapshot}
import com.sos.scheduler.engine.data.events.EventJsonFormat
import com.sos.scheduler.engine.data.jobchain.{JobChainDetails, JobChainOverview, JobChainPath}
import com.sos.scheduler.engine.data.order.OrderOverview
import com.sos.scheduler.engine.data.queries.{JobChainQuery, OrderQuery, PathQuery}
import com.sos.scheduler.engine.data.scheduler.SchedulerOverview
import scala.collection.immutable
import scala.collection.immutable.Seq
import scala.concurrent.Future
import spray.client.pipelining._
import spray.http.CacheDirectives.{`no-cache`, `no-store`}
import spray.http.HttpHeaders.{Accept, `Cache-Control`}
import spray.http.MediaTypes._
import spray.http._
import spray.httpx.SprayJsonSupport._
import spray.httpx.encoding.Gzip
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

  final def overview =
    get[Snapshot[SchedulerOverview]](_.overview)

  final def orderOverviewsBy(query: OrderQuery) =
    get[Snapshot[immutable.Seq[OrderOverview]]](_.order.overviews(query))

  final def orderTreeComplementedBy(query: OrderQuery) =
    get[Snapshot[OrderTreeComplemented]](_.order.treeComplemented(query))

  final def ordersComplementedBy(query: OrderQuery) =
    get[Snapshot[OrdersComplemented]](_.order.ordersComplemented(query))

  final def jobChainOverview(jobChainPath: JobChainPath) =
    get[Snapshot[JobChainOverview]](_.jobChain.overviews(JobChainQuery.Standard(PathQuery(jobChainPath))))

  final def jobChainOverviewsBy(query: JobChainQuery): Future[Snapshot[immutable.Seq[JobChainOverview]]] = {
    query.jobChainPathQuery match {
      case single: PathQuery.SinglePath ⇒
        for (schedulerResponse ← get[Snapshot[JobChainOverview]](_.jobChain.overview(single.as[JobChainPath])))
             yield for (o ← schedulerResponse)
          yield Vector(o) // Web service return a single object (not an array), if path denotes a single job chain path
      case _ ⇒
        get[Snapshot[immutable.Seq[JobChainOverview]]](_.jobChain.overviews(query))
    }
  }

  final def jobChainDetails(jobChainPath: JobChainPath) =
    get[Snapshot[JobChainDetails]](_.jobChain.details(jobChainPath))

  def events(after: EventId, limit: Int = Int.MaxValue, reverse: Boolean = false): Future[Snapshot[Seq[Snapshot[AnyKeyedEvent]]]] =
    get[Snapshot[immutable.Seq[Snapshot[AnyKeyedEvent]]]](_.events(after = after, limit = limit, reverse = reverse))

  final def getJson(pathUri: String): Future[String] =
    get[String](_.uriString(pathUri))

  final def get[A: FromResponseUnmarshaller](uri: SchedulerUris ⇒ String, accept: MediaType = `application/json`): Future[A] =
    unmarshallingPipeline[A](accept = accept).apply(Get(uri(uris)))

  private def unmarshallingPipeline[A: FromResponseUnmarshaller](accept: MediaType) =
    addHeader(Accept(accept)) ~> nonCachingHttpResponsePipeline ~> unmarshal[A]

  override def toString = s"WebSchedulerClient($uris)"
}
