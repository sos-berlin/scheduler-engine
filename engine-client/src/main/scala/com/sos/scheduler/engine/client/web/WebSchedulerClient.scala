package com.sos.scheduler.engine.client.web

import akka.util.Timeout
import com.sos.jobscheduler.base.generic.SecretString
import com.sos.jobscheduler.base.utils.ScalaUtils.implicitClass
import com.sos.jobscheduler.common.auth.{UserAndPassword, UserId}
import com.sos.jobscheduler.common.sprayutils.sprayclient.ExtendedPipelining.extendedSendReceive
import com.sos.jobscheduler.common.time.ScalaTime._
import com.sos.jobscheduler.data.agent.AgentAddress
import com.sos.jobscheduler.data.event.{Event, EventId, EventSeq, KeyedEvent, Stamped, SomeEventRequest}
import com.sos.jobscheduler.data.filebased.TypedPath
import com.sos.jobscheduler.data.job.TaskId
import com.sos.scheduler.engine.client.api.SchedulerClient
import com.sos.scheduler.engine.client.web.WebSchedulerClient._
import com.sos.scheduler.engine.data.compounds.{OrderTreeComplemented, OrdersComplemented}
import com.sos.scheduler.engine.data.events.SchedulerAnyKeyedEventJsonFormat.anyEventJsonFormat
import com.sos.scheduler.engine.data.events.schedulerKeyedEventJsonFormat
import com.sos.scheduler.engine.data.filebased.FileBasedView
import com.sos.scheduler.engine.data.job.{JobPath, JobView}
import com.sos.scheduler.engine.data.jobchain.{JobChainDetailed, JobChainOverview, JobChainPath}
import com.sos.scheduler.engine.data.order.{JocOrderStatistics, OrderKey, OrderView, Orders}
import com.sos.scheduler.engine.data.processclass.{ProcessClassPath, ProcessClassView}
import com.sos.scheduler.engine.data.queries.{JobChainNodeQuery, JobChainQuery, OrderQuery, PathQuery}
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
    get[Stamped[SchedulerOverview]](_.overview)

  def fileBased[P <: TypedPath, V <: FileBasedView: FileBasedView.Companion](path: P): Future[Stamped[V]] =
    get[Stamped[V]](_.fileBased(path, returnType = implicitly[FileBasedView.Companion[V]].name))

  def fileBaseds[P <: TypedPath: TypedPath.Companion, V <: FileBasedView: FileBasedView.Companion](query: PathQuery): Future[Stamped[immutable.Seq[V]]] =
    get[Stamped[immutable.Seq[V]]](_.fileBaseds[P](query, returnType = implicitly[FileBasedView.Companion[V]].name))

  def anyTypeFileBaseds[V <: FileBasedView: FileBasedView.Companion](query: PathQuery): Future[Stamped[immutable.Seq[V]]] =
    get[Stamped[immutable.Seq[V]]](_.anyTypeFileBaseds(query, returnType = implicitly[FileBasedView.Companion[V]].name))

  def fileBasedSourceXml[P <: TypedPath, V: FromResponseUnmarshaller](path: P): Future[V] =
    get[V](_.fileBased(path, returnType = "FileBasedSource"), accept = `application/xml`)

  // Order

  final def order[V <: OrderView: OrderView.Companion](orderKey: OrderKey): Future[Stamped[V]] =
    get[Stamped[V]](_.order[V](orderKey))

  final def ordersBy[V <: OrderView: OrderView.Companion](query: OrderQuery): Future[Stamped[immutable.Seq[V]]] =
    post[OrderQuery, Stamped[Orders[V]]](_.order.forPost[V], query) map { _ map { _.orders }}

  final def getOrdersBy[V <: OrderView: OrderView.Companion](query: OrderQuery): Future[Stamped[immutable.Seq[V]]] =
    get[Stamped[Orders[V]]](_.order[V](query)) map { _ map { _.orders }}

  final def orderTreeComplementedBy[V <: OrderView: OrderView.Companion](query: OrderQuery) =
    post[OrderQuery, Stamped[OrderTreeComplemented[V]]](_.order.treeComplementedForPost[V], query)

  final def getOrderTreeComplementedBy[V <: OrderView: OrderView.Companion](query: OrderQuery) =
    get[Stamped[OrderTreeComplemented[V]]](_.order.treeComplemented[V](query))

  final def ordersComplementedBy[V <: OrderView: OrderView.Companion](query: OrderQuery) =
    post[OrderQuery, Stamped[OrdersComplemented[V]]](_.order.complementedForPost[V], query)

  final def getOrdersComplementedBy[V <: OrderView: OrderView.Companion](query: OrderQuery) =
    get[Stamped[OrdersComplemented[V]]](_.order.complemented[V](query))

  final def jocOrderStatistics(query: JobChainNodeQuery): Future[Stamped[JocOrderStatistics]] =
    post[JobChainNodeQuery, Stamped[JocOrderStatistics]](_.order.jocOrderStatisticsForPost(query), query)

  final def orderEvents[E <: Event: ClassTag](query: OrderQuery, after: EventId, timeout: Duration, limit: Int = Int.MaxValue): Future[Stamped[EventSeq[Seq, KeyedEvent[E]]]] =
    get[Stamped[EventSeq[Seq, KeyedEvent[E]]]](_.order.events(query, after = after, timeout, limit = limit, returnType = implicitClass[E].getSimpleName))

  // JobChain

  final def jobChainOverview(jobChainPath: JobChainPath) =
    get[Stamped[JobChainOverview]](_.jobChain.overviews(JobChainQuery(PathQuery(jobChainPath))))

  final def jobChainOverviewsBy(query: JobChainQuery): Future[Stamped[immutable.Seq[JobChainOverview]]] =
    query.pathQuery match {
      case single: PathQuery.SinglePath ⇒
        for (schedulerResponse ← get[Stamped[JobChainOverview]](_.jobChain.overview(single.as[JobChainPath]))) yield
          for (o ← schedulerResponse) yield
            Vector(o) // Web service returns a single object (not an array), if path designate a single job chain path
      case _ ⇒
        get[Stamped[immutable.Seq[JobChainOverview]]](_.jobChain.overviews(query))
    }

  final def jobChainDetailed(jobChainPath: JobChainPath) =
    get[Stamped[JobChainDetailed]](_.jobChain.detailed(jobChainPath))

  final def jobChainEvents[E <: Event](jobChainPath: JobChainPath, eventRequest: SomeEventRequest[E]) =
    get[Stamped[EventSeq[Seq, E]]](_.jobChain.events(jobChainPath, eventRequest))

  final def jobChainEventsBy[E <: Event](query: JobChainQuery, eventRequest: SomeEventRequest[E]) = {
    require(!query.pathQuery.isInstanceOf[PathQuery.SinglePath], "SinglePath not allowed here")  // Because it would return E, not KeyedEvent[E].
    get[Stamped[EventSeq[Seq, KeyedEvent[E]]]](_.jobChain.events(query, eventRequest))
  }

  // Job

  def job[V <: JobView: JobView.Companion](path: JobPath) =
    get[Stamped[V]](_.job[V](path))

  def jobs[V <: JobView: JobView.Companion](query: PathQuery) =
    get[Stamped[immutable.Seq[V]]](_.job[V](query))

  def jobEvents[E <: Event](jobPath: JobPath, eventRequest: SomeEventRequest[E]) =
    get[Stamped[EventSeq[Seq, E]]](_.job.events(PathQuery(jobPath), eventRequest))

  def jobEventsBy[E <: Event](query: PathQuery, eventRequest: SomeEventRequest[E]) =
    get[Stamped[EventSeq[Seq, KeyedEvent[E]]]](_.job.events(query, eventRequest))

  def taskEvents[E <: Event](taskId: TaskId, eventRequest: SomeEventRequest[E]) =
    get[Stamped[EventSeq[Seq, E]]](_.task.events(taskId, eventRequest))

  def taskEventsBy[E <: Event](query: PathQuery, eventRequest: SomeEventRequest[E]) =
    get[Stamped[EventSeq[Seq, KeyedEvent[E]]]](_.task.eventsBy(query, eventRequest))

  // ProcessClass

  final def processClass[V <: ProcessClassView: ProcessClassView.Companion](processClassPath: ProcessClassPath) =
    get[Stamped[V]](_.processClass.view[V](processClassPath))

  final def processClasses[V <: ProcessClassView: ProcessClassView.Companion](query: PathQuery) =
    get[Stamped[immutable.Seq[V]]](_.processClass.views[V](query))

  // Agent

  final def agentUris: Future[Stamped[Set[AgentAddress]]] =
    get[Stamped[Set[AgentAddress]]](_.agent.agentUris)

  final def agentGet[A: FromResponseUnmarshaller](uri: String): Future[A] =
    get[A](_.agent.forward(uri))

  // Event

  final def events[E <: Event](request: SomeEventRequest[E]): Future[Stamped[EventSeq[Seq, KeyedEvent[E]]]] =
    get[Stamped[EventSeq[Seq, KeyedEvent[E]]]](_.events(request))

  final def eventsByPath[E <: Event](request: SomeEventRequest[E], query: PathQuery): Future[Stamped[EventSeq[Seq, KeyedEvent[E]]]] =
    get[Stamped[EventSeq[Seq, KeyedEvent[E]]]](_.eventsByPath(request, query))

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

object WebSchedulerClient {
  private val timeout = Timeout(3600.s.toFiniteDuration)
}
