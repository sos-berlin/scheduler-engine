package com.sos.scheduler.engine.client.web

import akka.util.Timeout
import com.sos.scheduler.engine.base.generic.SecretString
import com.sos.scheduler.engine.base.utils.ScalaUtils.implicitClass
import com.sos.scheduler.engine.client.api.SchedulerClient
import com.sos.scheduler.engine.client.web.WebSchedulerClient._
import com.sos.scheduler.engine.common.auth.{UserAndPassword, UserId}
import com.sos.scheduler.engine.common.sprayutils.sprayclient.ExtendedPipelining.extendedSendReceive
import com.sos.scheduler.engine.common.time.ScalaTime._
import com.sos.scheduler.engine.data.agent.AgentAddress
import com.sos.scheduler.engine.data.compounds.{OrderTreeComplemented, OrdersComplemented}
import com.sos.scheduler.engine.data.event.{Event, EventId, EventSeq, KeyedEvent, Snapshot, SomeEventRequest}
import com.sos.scheduler.engine.data.events.SchedulerAnyKeyedEventJsonFormat.anyEventJsonFormat
import com.sos.scheduler.engine.data.events.schedulerKeyedEventJsonFormat
import com.sos.scheduler.engine.data.filebased.{FileBasedView, TypedPath}
import com.sos.scheduler.engine.data.job.{JobPath, JobState, JobView, TaskId}
import com.sos.scheduler.engine.data.jobchain.{JobChainDetailed, JobChainOverview, JobChainPath}
import com.sos.scheduler.engine.data.order.{JocOrderStatistics, OrderKey, OrderView, Orders}
import com.sos.scheduler.engine.data.processclass.{ProcessClassPath, ProcessClassView}
import com.sos.scheduler.engine.data.queries.{JobChainNodeQuery, JobChainQuery, JobQuery, OrderQuery, PathQuery}
import com.sos.scheduler.engine.data.scheduler.SchedulerOverview
import java.time.Duration
import scala.collection.immutable
import scala.collection.immutable.Seq
import scala.concurrent.Future
import scala.reflect.ClassTag
import spray.can.Http.HostConnectorSetup
import spray.client.pipelining._
import spray.http.HttpHeaders.Accept
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

  protected def hostConnectorSetupOption: Option[HostConnectorSetup]
  protected def credentials: Option[UserAndPassword] = None

  import actorRefFactory.dispatcher


  protected final def commandUri = uris.command

  def uris: SchedulerUris

  private lazy val httpResponsePipeline = {
    val credentialsHeader: RequestTransformer = credentials match {
      case Some(UserAndPassword(UserId(user), SecretString(pass))) ⇒ addCredentials(BasicHttpCredentials(user, pass))
      case None ⇒ identity
    }
    credentialsHeader ~>
      encode(Gzip) ~>
      extendedSendReceive(timeout, hostConnectorSetupOption) ~>
      decode(Gzip)
  }

  private lazy val jsonHttpResponsePipeline =
    addHeader(Accept(`application/json`)) ~>
      httpResponsePipeline

  final def overview =
    get[Snapshot[SchedulerOverview]](_.overview)

  def fileBased[P <: TypedPath, V <: FileBasedView: FileBasedView.Companion](path: P): Future[Snapshot[V]] =
    get[Snapshot[V]](_.fileBased(path, returnType = implicitly[FileBasedView.Companion[V]].name))

  def fileBaseds[P <: TypedPath: TypedPath.Companion, V <: FileBasedView: FileBasedView.Companion](query: PathQuery): Future[Snapshot[immutable.Seq[V]]] =
    get[Snapshot[immutable.Seq[V]]](_.fileBaseds[P](query, returnType = implicitly[FileBasedView.Companion[V]].name))

  def anyTypeFileBaseds[V <: FileBasedView: FileBasedView.Companion](query: PathQuery): Future[Snapshot[immutable.Seq[V]]] =
    get[Snapshot[immutable.Seq[V]]](_.anyTypeFileBaseds(query, returnType = implicitly[FileBasedView.Companion[V]].name))

  def fileBasedSourceXml[P <: TypedPath, V: FromResponseUnmarshaller](path: P): Future[V] =
    get[V](_.fileBased(path, returnType = "FileBasedSource"), accept = `application/xml`)

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

  final def jocOrderStatistics(query: JobChainNodeQuery): Future[Snapshot[JocOrderStatistics]] =
    post[JobChainNodeQuery, Snapshot[JocOrderStatistics]](_.order.jocOrderStatisticsForPost(query), query)

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
        post[JobChainQuery, Snapshot[immutable.Seq[JobChainOverview]]](_.jobChain.forPost(returnType = "JobChainOverview"), query)
    }

  final def jobChainDetailed(jobChainPath: JobChainPath) =
    get[Snapshot[JobChainDetailed]](_.jobChain.detailed(jobChainPath))

  final def jobChainDetailedBy(query: JobChainQuery): Future[Snapshot[immutable.Seq[JobChainDetailed]]] =
    query.pathQuery match {
      case single: PathQuery.SinglePath ⇒
        for (schedulerResponse ← get[Snapshot[JobChainDetailed]](_.jobChain.detailed(single.as[JobChainPath]))) yield
          for (o ← schedulerResponse) yield
            Vector(o) // Web service returns a single object (not an array), if path designate a single job chain path
      case _ ⇒
        post[JobChainQuery, Snapshot[immutable.Seq[JobChainDetailed]]](_.jobChain.forPost(returnType = "JobChainDetailed"), query)
    }

  final def jobChainEvents[E <: Event](jobChainPath: JobChainPath, eventRequest: SomeEventRequest[E]) =
    get[Snapshot[EventSeq[Seq, E]]](_.jobChain.events(jobChainPath, eventRequest))

  final def jobChainEventsBy[E <: Event](query: JobChainQuery, eventRequest: SomeEventRequest[E]) = {
    require(!query.pathQuery.isInstanceOf[PathQuery.SinglePath], "SinglePath not allowed here")  // Because it would return E, not KeyedEvent[E].
    get[Snapshot[EventSeq[Seq, KeyedEvent[E]]]](_.jobChain.events(query, eventRequest))
  }

  // Job

  def job[V <: JobView: JobView.Companion](path: JobPath) =
    get[Snapshot[V]](_.job[V](path))

  def jobs[V <: JobView: JobView.Companion](query: JobQuery) = {
    require(JobState.values forall query.isInState, "JobQuery.isInState not implemented")
    implicit val x = PathQuery.jsonFormat[JobPath]
    post[PathQuery, Snapshot[immutable.Seq[V]]](
      uri = _.job.forPost(returnType = implicitly[JobView.Companion[V]].name),
      data = query.pathQuery)
  }

  def jobEvents[E <: Event](jobPath: JobPath, eventRequest: SomeEventRequest[E]) =
    get[Snapshot[EventSeq[Seq, E]]](_.job.events(PathQuery(jobPath), eventRequest))

  def jobEventsBy[E <: Event](query: PathQuery, eventRequest: SomeEventRequest[E]) =
    get[Snapshot[EventSeq[Seq, KeyedEvent[E]]]](_.job.events(query, eventRequest))

  def taskEvents[E <: Event](taskId: TaskId, eventRequest: SomeEventRequest[E]) =
    get[Snapshot[EventSeq[Seq, E]]](_.task.events(taskId, eventRequest))

  def taskEventsBy[E <: Event](query: PathQuery, eventRequest: SomeEventRequest[E]) =
    get[Snapshot[EventSeq[Seq, KeyedEvent[E]]]](_.task.eventsBy(query, eventRequest))

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

  final def events[E <: Event](request: SomeEventRequest[E]): Future[Snapshot[EventSeq[Seq, KeyedEvent[E]]]] =
    get[Snapshot[EventSeq[Seq, KeyedEvent[E]]]](_.events(request))

  final def eventsByPath[E <: Event](request: SomeEventRequest[E], query: PathQuery): Future[Snapshot[EventSeq[Seq, KeyedEvent[E]]]] =
    get[Snapshot[EventSeq[Seq, KeyedEvent[E]]]](_.eventsByPath(request, query))

  // Basic

  final def getByUri[A: FromResponseUnmarshaller](relativeUri: String, accept: MediaType = `application/json`): Future[A] =
    get[A](_.uriString(relativeUri), accept)

  final def postByUri[A: Marshaller, B: FromResponseUnmarshaller](relativeUri: String, data: A): Future[B] =
    post[A, B](_.uriString(relativeUri), data)

  final def get[A: FromResponseUnmarshaller](uri: SchedulerUris ⇒ String, accept: MediaType = `application/json`): Future[A] =
    if (accept == `application/json`)
      jsonUnmarshallingPipeline[A].apply(Get(uri(uris)))
    else
      unmarshallingPipeline[A](accept = accept).apply(Get(uri(uris)))

  final def getHttpResponse(uri: SchedulerUris ⇒ String, accept: MediaRange): Future[HttpResponse] =
    (addHeader(Accept(accept)) ~> httpResponsePipeline)
      .apply(Get(uri(uris)))

  private def unmarshallingPipeline[A: FromResponseUnmarshaller](accept: MediaType) =
    addHeader(Accept(accept)) ~> httpResponsePipeline ~> unmarshal[A]

  final def putRaw(uri: SchedulerUris ⇒ String, content: HttpEntity): Future[HttpResponse] =
    httpResponsePipeline.apply(Put(uri(uris), content))

  final def delete(uri: SchedulerUris ⇒ String): Future[HttpResponse] =
    httpResponsePipeline.apply(Delete(uri(uris)))

  private final def post[A: Marshaller, B: FromResponseUnmarshaller](uri: SchedulerUris ⇒ String, data: A): Future[B] =
    jsonUnmarshallingPipeline[B].apply(Post(uri(uris), data))

  private def jsonUnmarshallingPipeline[A: FromResponseUnmarshaller] =
    jsonHttpResponsePipeline ~> unmarshal[A]

  override def toString = s"WebSchedulerClient($uris)"
}

object WebSchedulerClient {
  private val timeout = Timeout(3600.s.toFiniteDuration)
}
