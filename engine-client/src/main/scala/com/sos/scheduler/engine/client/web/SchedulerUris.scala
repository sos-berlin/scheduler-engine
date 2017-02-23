package com.sos.scheduler.engine.client.web

import com.sos.jobscheduler.base.serial.PathAndParameterSerializable
import com.sos.jobscheduler.base.serial.PathAndParameterSerializable.toPathAndParameters
import com.sos.jobscheduler.common.scalautil.Collections._
import com.sos.jobscheduler.data.event.{Event, EventId, SomeEventRequest}
import com.sos.jobscheduler.data.filebased.TypedPath
import com.sos.jobscheduler.data.job.TaskId
import com.sos.scheduler.engine.client.web.SchedulerUris._
import com.sos.scheduler.engine.data.job.{JobPath, JobView}
import com.sos.scheduler.engine.data.jobchain.JobChainPath
import com.sos.scheduler.engine.data.order.{OrderKey, OrderView}
import com.sos.scheduler.engine.data.processclass.{ProcessClassPath, ProcessClassView}
import com.sos.scheduler.engine.data.queries.{JobChainNodeQuery, JobChainQuery, OrderQuery, PathQuery}
import java.time.Duration
import scala.language.reflectiveCalls
import spray.http.Uri

/**
  * @author Joacim Zschimmer
  */
final class SchedulerUris private(schedulerUri: Uri) {

  private[web] val masterUri = Uri(s"$schedulerUri/master/")

  lazy val command = resolveMasterUri("api/command")

  lazy val overview = uriString("api")

  def fileBased[P <: TypedPath](path: P, returnType: String) =
    uriString(Uri.Path("api/" + path.companion.lowerCaseCamelName + path.string), "return" → returnType)

  def fileBaseds[P <: TypedPath: TypedPath.Companion](query: PathQuery, returnType: String) =
    uriString(Uri.Path("api/" + implicitly[TypedPath.Companion[P]].lowerCaseCamelName + query.toUriPath), "return" → returnType)

  def anyTypeFileBaseds(query: PathQuery, returnType: String) =
    uriString(Uri.Path("api/fileBased" + query.toUriPath), "return" → returnType)

  object order {
    def apply[V <: OrderView: OrderView.Companion](orderKey: OrderKey): String =
      uriString(Uri.Path("api/order" + orderKey.string), "return" → orderView[V].name)

    def apply[V <: OrderView: OrderView.Companion]: String =
      apply[V](OrderQuery.All)

    def apply[V <: OrderView: OrderView.Companion](query: OrderQuery): String =
      forGet(query, returnType = Some(orderView[V].name))

    def apply(query: OrderQuery, returnType: Option[String]): String =
      forGet(query, returnType)

    def treeComplemented[V <: OrderView: OrderView.Companion]: String =
      treeComplemented[V](OrderQuery.All)

    def treeComplemented[V <: OrderView: OrderView.Companion](query: OrderQuery): String =
      forGet(query, Some(s"OrderTreeComplemented/${orderView[V].name}"))

    def treeComplementedForPost[V <: OrderView: OrderView.Companion]: String =
      forPost(returnType = Some(s"OrderTreeComplemented/${orderView[V].name}"))

    def complemented[V <: OrderView: OrderView.Companion](query: OrderQuery = OrderQuery.All): String =
      forGet(query, Some(s"OrdersComplemented/${orderView.name}"))

    def complementedForPost[V <: OrderView: OrderView.Companion]: String =
      forPost(returnType = Some(s"OrdersComplemented/${orderView.name}"))

    def getJocOrderStatistics(query: JobChainQuery = JobChainQuery.All): String =
      forGet(query, returnType = Some("JocOrderStatistics"))

    def jocOrderStatisticsForPost(query: JobChainNodeQuery = JobChainNodeQuery.All): String =
      forPost(returnType = Some("JocOrderStatistics"))

    private def forGet[A: PathAndParameterSerializable](query: A, returnType: Option[String]): String = {
      val (path, parameters) = toPathAndParameters(query)
      uriString(Uri(
        path = Uri.Path(s"api/order$path"),
        query = Uri.Query(parameters ++ (returnType map { o ⇒ "return" → o }))))
    }

    def forPost[V <: OrderView: OrderView.Companion]: String =
      forPost(returnType = Some(orderView[V].name))

    private def forPost(returnType: Option[String]): String = {
      uriString(Uri(
        path = Uri.Path(s"api/order"),
        query = Uri.Query((returnType map { o ⇒ "return" → o }).toMap)))
    }

    def events(query: OrderQuery, after: EventId, timeout: Duration, limit: Int, returnType: String = DefaultEventName) = {
      val (path, parameters) = toPathAndParameters(query)
      uriString(Uri(
        path = Uri.Path(s"api/order$path"),
        query = Uri.Query(parameters + ("after" → after.toString) + ("timeout" → timeout.toString) + ("limit" → limit.toString) + ("return" → returnType))))
    }

    private def orderView[V <: OrderView: OrderView.Companion] =
      implicitly[OrderView.Companion[V]]
  }

  object jobChain {
    def overviews(query: JobChainQuery = JobChainQuery.All): String = {
      val (subpath, parameters) = query.toPathAndParameters
      require(subpath endsWith "/", "JobChainQuery must denote folder, terminated by a slash")
      uriString(Uri.Path(s"api/jobChain$subpath"), parameters.toSeq: _*)  // Default with trailing slash: query = Uri.Query("return" → "JobChainOverview")))
    }

    def overview(jobChainPath: JobChainPath): String = {
      val (subpath, parameters) = JobChainQuery(jobChainPath).toPathAndParameters
      require(!subpath.endsWith("/"), "Invalid JobChainPath has trailing slash")
      uriString(Uri.Path(s"api/jobChain$subpath"), parameters + ("return" → "JobChainOverview"))
    }

    def detailed(jobChainPath: JobChainPath): String = {
      val (subpath, parameters) = JobChainQuery(jobChainPath).toPathAndParameters
      require(!subpath.endsWith("/"), "Invalid JobChainPath has trailing slash")
      uriString(Uri.Path(s"api/jobChain$subpath"), parameters)  // Default without trailing slash: query = Uri.Query("return" → "JobChainDetailed")))
    }

    def events[E <: Event](query: JobChainQuery, eventRequest: SomeEventRequest[E]): String = {
      val (subpath, parameters) = query.toPathAndParameters
      uriString(Uri.Path(s"api/jobChain$subpath"), parameters ++ eventRequest.toQueryParameters)
    }
  }

  object job {
    def apply[V <: JobView: JobView.Companion](path: JobPath): String =
      apply[V](PathQuery(path))

    def apply[V <: JobView: JobView.Companion](query: PathQuery): String =
      uriString(Uri.Path("api/job" + query.toUriPath), "return" → JobView.companion[V].name)

    def events[E <: Event](query: PathQuery, eventRequest: SomeEventRequest[E]): String = {
      val (subpath, parameters) = query.toPathAndParameters[JobPath]
      uriString(Uri.Path(s"api/job$subpath"), parameters ++ eventRequest.toQueryParameters)
    }
  }

  object processClass {
    private val pathAndParameterSerializable = PathQuery.pathAndParameterSerializable[ProcessClassPath]

    def view[V <: ProcessClassView: ProcessClassView.Companion](processClassPath: ProcessClassPath): String = {
      val (subpath, parameters) = pathAndParameterSerializable.toPathAndParameters(processClassPath)
      uriString(Uri.Path(s"api/processClass$subpath"), parameters + ("return" → implicitly[ProcessClassView.Companion[V]].name))
    }

    def views[V <: ProcessClassView: ProcessClassView.Companion](query: PathQuery): String = {
      val (subpath, parameters) = query.toPathAndParameters[ProcessClassPath]
      require(subpath.endsWith("/"), "PathQuery needs trailing slash here")
      uriString(Uri.Path(s"api/processClass$subpath"), parameters + ("return" → implicitly[ProcessClassView.Companion[V]].name))
    }
  }

  object task {
    def overview(taskId: TaskId) = uriString(Uri.Path("api/task") / taskId.string)

    def events[E <: Event](taskId: TaskId, request: SomeEventRequest[E]): String =
      uriString(Uri.Path(s"api/task"), ("taskId" → taskId.string) +: request.toQueryParameters: _*)

    def eventsBy[E <: Event](query: PathQuery, request: SomeEventRequest[E]): String = {
      val (subpath, parameters) = query.toPathAndParameters[JobPath]
      uriString(Uri.Path(s"api/task$subpath"), request.toQueryParameters: _*)
    }
  }

  object agent {
    def agentUris = uriString(Uri.Path("api/agent/"))

    def forward(agentUri: String) = uriString(Uri.Path(s"api/agent/$agentUri"))
  }

  object events {
    private val DefaultParam = "return" → "Event"

    def apply[E <: Event](request: SomeEventRequest[E]) =
      uriString(Uri.Path("api/event"), request.toQueryParameters filter DefaultParam.!= : _*)
  }

  def eventsByPath[E <: Event](request: SomeEventRequest[E], query: PathQuery) = {
    val (subpath, queryParams) = query.toPathAndParameters[ProcessClassPath]
    uriString(Uri.Path(s"api/fileBased$subpath"), queryParams.toSeq ++ request.toQueryParameters: _*)
  }

  def uriString(path: Uri.Path, parameters: (String, String)*): String =
    resolveMasterUri(Uri(path = path, query = Uri.Query(parameters: _*))).toString

  def uriString(path: Uri.Path, parameters: Map[String, String]): String =
    resolveMasterUri(Uri(path = path, query = Uri.Query(parameters))).toString

  def uriString(relativeUri: Uri): String = resolveMasterUri(relativeUri).toString

  def /(relativeUri: Uri): Uri = resolveMasterUri(relativeUri)

  private def resolveMasterUri(relativeUri: Uri): Uri = resolveUri(relativeUri, masterUri)

  override def toString = s"SchedulerUris($schedulerUri)"
}

object SchedulerUris {
  val DefaultEventName = "Event"

  def apply(schedulerUri: Uri): SchedulerUris = {
    require(!schedulerUri.isEmpty, "JobScheduler URI is empty")
    new SchedulerUris(schedulerUri.copy(path = normalizeSchedulerRootPath(schedulerUri.path)))
  }

  private def normalizeSchedulerRootPath(path: Uri.Path): Uri.Path = {
    val result = path.toString stripSuffix "/" match {
      case "" ⇒ "/jobscheduler"  // Default prefix
      case o ⇒ o
    }
    Uri.Path(result)
  }


  private[web] def resolveUri(uri: Uri, against: Uri) = {
    val scheme = emptyToNone(uri.scheme) getOrElse against.scheme
    val dummyAbsolute = Uri("dummy:")
    uri.resolvedAgainst(against resolvedAgainst dummyAbsolute).withScheme(scheme)
  }
}
