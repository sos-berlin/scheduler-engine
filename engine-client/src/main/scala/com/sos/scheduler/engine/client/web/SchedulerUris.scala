package com.sos.scheduler.engine.client.web

import com.sos.scheduler.engine.base.serial.PathAndParameterSerializable
import com.sos.scheduler.engine.base.serial.PathAndParameterSerializable.toPathAndParameters
import com.sos.scheduler.engine.client.web.SchedulerUris._
import com.sos.scheduler.engine.common.scalautil.Collections._
import com.sos.scheduler.engine.common.time.ScalaTime._
import com.sos.scheduler.engine.data.event.EventId
import com.sos.scheduler.engine.data.filebased.TypedPath
import com.sos.scheduler.engine.data.job.{JobPath, JobView, TaskId}
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

  def fileBasedDetailed[P <: TypedPath: TypedPath.Companion](path: P) = {
    val companion = implicitly[TypedPath.Companion[P]]
    uriString(Uri.Path("api/" + companion.fileBasedType.lowerCaseCamelName + path.string), "return" → "FileBasedDetailed")
  }

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

    def getStatistics(query: JobChainQuery = JobChainQuery.All): String =
      forGet(query, returnType = Some("OrderStatistics"))

    def statisticsForPost(query: JobChainNodeQuery = JobChainNodeQuery.All): String =
      forPost(returnType = Some("OrderStatistics"))

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
  }

  object job {
    def apply[V <: JobView: JobView.Companion](path: JobPath): String =
      apply[V](PathQuery(path))

    def apply[V <: JobView: JobView.Companion](query: PathQuery): String =
      uriString(Uri.Path("api/job" + query.toUriPath), "return" → JobView.companion[V].name)
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
  }

  object agent {
    def agentUris = uriString(Uri.Path("api/agent/"))

    def forward(agentUri: String) = uriString(Uri.Path(s"api/agent/$agentUri"))
  }

  def events(after: EventId, timeout: Duration, limit: Int = Int.MaxValue, returnType: String = DefaultEventName) = {
    require(limit > 0, "Limit must not be below zero")
    val params = Vector.build[(String, String)] { builder ⇒
      if (returnType != DefaultEventName) builder += "return" → returnType
      if (limit != Int.MaxValue) builder += "limit" → limit.toString
      if (timeout != 0.s) builder += "timeout" → timeout.toSecondsString
      builder += "after" → after.toString
    }
    uriString(Uri.Path("api/event"), params: _*)
  }

  def eventsReverse(after: EventId = EventId.BeforeFirst, limit: Int, returnType: String = DefaultEventName) = {
    require(limit > 0, "Limit must not be below zero")
    val params = Vector.build[(String, String)] { builder ⇒
      if (returnType != DefaultEventName) builder += "return" → returnType
      if (limit != Int.MaxValue) builder += "limit" → (-limit).toString
      if (after != EventId.BeforeFirst) builder += "after" → after.toString
    }
    uriString(Uri.Path("api/event"), params: _*)
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
