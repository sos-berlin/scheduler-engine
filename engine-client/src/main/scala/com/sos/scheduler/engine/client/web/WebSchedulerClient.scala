package com.sos.scheduler.engine.client.web

import com.sos.scheduler.engine.client.api.SchedulerClient
import com.sos.scheduler.engine.data.compounds.{OrderTreeComplemented, OrdersComplemented}
import com.sos.scheduler.engine.data.jobchain.{JobChainDetails, JobChainOverview, JobChainPath}
import com.sos.scheduler.engine.data.order.OrderOverview
import com.sos.scheduler.engine.data.queries.{JobChainQuery, OrderQuery, PathQuery}
import com.sos.scheduler.engine.data.scheduler.SchedulerOverview
import scala.collection.immutable
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

  final def overview: Future[SchedulerOverview] =
    get[SchedulerOverview](_.overview)

  final def orderOverviewsBy(query: OrderQuery): Future[immutable.Seq[OrderOverview]] =
    get[immutable.Seq[OrderOverview]](_.order.overviews(query))

  final def orderTreeComplementedBy(query: OrderQuery) =
    get[OrderTreeComplemented](_.order.treeComplemented(query))

  final def ordersComplementedBy(query: OrderQuery): Future[OrdersComplemented] =
    get[OrdersComplemented](_.order.ordersComplemented(query))

  final def jobChainOverview(jobChainPath: JobChainPath): Future[JobChainOverview] =
    get[JobChainOverview](_.jobChain.overviews(JobChainQuery.Standard(PathQuery(jobChainPath))))

  final def jobChainOverviewsBy(query: JobChainQuery): Future[immutable.Seq[JobChainOverview]] = {
    query.jobChainPathQuery match {
      case single: PathQuery.SinglePath ⇒ get[JobChainOverview](_.jobChain.overview(single.as[JobChainPath])) map { o ⇒ Vector(o) }  // Web service return a single object (not an array), if path denotes a single job chain path
      case _ ⇒ get[immutable.Seq[JobChainOverview]](_.jobChain.overviews(query))
    }
  }

  final def jobChainDetails(jobChainPath: JobChainPath) =
    get[JobChainDetails](_.jobChain.details(jobChainPath))


  final def getJson(pathUri: String): Future[String] =
    get[String](_.resolvePathUri(pathUri).toString)

  final def get[A: FromResponseUnmarshaller](uri: SchedulerUris ⇒ String, accept: MediaType = `application/json`) =
    unmarshallingPipeline[A](accept = accept).apply(Get(uri(uris)))

  private def unmarshallingPipeline[A: FromResponseUnmarshaller](accept: MediaType) =
    addHeader(Accept(accept)) ~> nonCachingHttpResponsePipeline ~> unmarshal[A]

  override def toString = s"WebSchedulerClient($uris)"
}
