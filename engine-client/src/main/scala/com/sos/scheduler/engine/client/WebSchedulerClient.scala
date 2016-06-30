package com.sos.scheduler.engine.client

import akka.actor.{ActorRefFactory, ActorSystem}
import com.sos.scheduler.engine.data.order.OrderOverview
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
trait WebSchedulerClient extends SchedulerClient {
  import actorRefFactory.dispatcher

  protected val schedulerUri: String
  protected implicit val actorRefFactory: ActorRefFactory

  lazy val uris = SchedulerUris(schedulerUri)
  private lazy val nonCachingHttpResponsePipeline: HttpRequest ⇒ Future[HttpResponse] =
    addHeader(Accept(`application/json`)) ~>
    addHeader(`Cache-Control`(`no-cache`, `no-store`)) ~>
    encode(Gzip) ~>
    sendReceive ~>
    decode(Gzip)

  final def overview: Future[SchedulerOverview] = get[SchedulerOverview](_.overview)

  final def orderOverviews: Future[immutable.Seq[OrderOverview]] = get[immutable.Seq[OrderOverview]](_.orderOverviews)

  final def get[A: FromResponseUnmarshaller](uri: SchedulerUris ⇒ String): Future[A] =
    unmarshallingPipeline[A].apply(Get(uri(uris)))

  private def unmarshallingPipeline[A: FromResponseUnmarshaller] = nonCachingHttpResponsePipeline ~> unmarshal[A]

  override def toString = s"WebSchedulerClient($schedulerUri)"
}

object WebSchedulerClient {
  def apply(schedulerUri: String): WebSchedulerClient with AutoCloseable = new Standard(schedulerUri)

  final class Standard(protected val schedulerUri: String) extends WebSchedulerClient with AutoCloseable {
    private val actorSystem = ActorSystem("WebSchedulerClient")
    protected val actorRefFactory = actorSystem

    def close() = {
      actorSystem.shutdown()
      actorSystem.awaitTermination()
    }
  }
}
