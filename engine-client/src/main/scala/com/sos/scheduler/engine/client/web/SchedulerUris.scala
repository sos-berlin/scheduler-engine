package com.sos.scheduler.engine.client.web

import com.sos.scheduler.engine.base.serial.PathAndParameterSerializable
import com.sos.scheduler.engine.base.serial.PathAndParameterSerializable.toPathAndParameters
import com.sos.scheduler.engine.base.utils.ScalazStyle.OptionRichBoolean
import com.sos.scheduler.engine.client.web.SchedulerUris._
import com.sos.scheduler.engine.client.web.common.PathQueryHttp
import com.sos.scheduler.engine.common.scalautil.Collections._
import com.sos.scheduler.engine.data.event.EventId
import com.sos.scheduler.engine.data.filebased.TypedPath
import com.sos.scheduler.engine.data.job.{JobPath, TaskId}
import com.sos.scheduler.engine.data.jobchain.JobChainPath
import com.sos.scheduler.engine.data.order.{OrderKey, OrderView}
import com.sos.scheduler.engine.data.processclass.{ProcessClassPath, ProcessClassView}
import com.sos.scheduler.engine.data.queries.{JobChainNodeQuery, JobChainQuery, OrderQuery, PathQuery}
import scala.language.reflectiveCalls
import spray.http.Uri

/**
  * @author Joacim Zschimmer
  */
final class SchedulerUris private(schedulerUriString: String) {

  private val rootUri = Uri(s"$schedulerUriString/$Prefix/")

  lazy val command = resolvePathUri("api/command")

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

    def events(orderKey: OrderKey) =
      uriString(Uri.Path("api/order" + orderKey.string), "return" → "Event")

    private def orderView[V <: OrderView: OrderView.Companion] =
      implicitly[OrderView.Companion[V]]
  }

  object jobChain {
    def overviews(query: JobChainQuery = JobChainQuery.All): String = {
      val subpath = query.pathQuery.toUriPath
      require(subpath endsWith "/", "JobChainQuery must denote folder, terminated by a slash")
      uriString(Uri.Path(s"api/jobChain$subpath"))  // Default with trailing slash: query = Uri.Query("return" → "JobChainOverview")))
    }

    def overview(jobChainPath: JobChainPath): String = {
      val subpath = PathQueryHttp.toUriPath(PathQuery(jobChainPath))
      require(!subpath.endsWith("/"), "Invalid JobChainPath has trailing slash")
      uriString(Uri.Path(s"api/jobChain$subpath"), "return" → "JobChainOverview")
    }

    def details(jobChainPath: JobChainPath): String = {
      val subpath = PathQueryHttp.toUriPath(PathQuery(jobChainPath))
      require(!subpath.endsWith("/"), "Invalid JobChainPath has trailing slash")
      uriString(Uri.Path(s"api/jobChain$subpath"))  // Default without trailing slash: query = Uri.Query("return" → "JobChainDetailed")))
    }
  }

  object job {
    def overviews(): String = uriString(Uri.Path(s"api/job/"))
//    def overviews(query: JobQuery = JobQuery.All): String = {
//      val subpath = JobQueryHttp.toUriPath(query)
//      require(subpath endsWith "/", "JobQuery must denote folder, terminated by a slash")
//      uriString(Uri(path = Uri.Path(s"api/job$subpath")))  // Default with trailing slash: query = Uri.Query("return" → "JobOverview")))
//    }

    def overview(jobPath: JobPath): String = {
      val subpath = PathQueryHttp.toUriPath(PathQuery(jobPath))
      require(!subpath.endsWith("/"), "Invalid JobPath has trailing slash")
      uriString(Uri.Path(s"api/job$subpath"), "return" → "JobOverview")
    }

    def details(jobPath: JobPath): String = {
      val subpath = PathQueryHttp.toUriPath(PathQuery(jobPath))
      require(!subpath.endsWith("/"), "Invalid JobPath has trailing slash")
      uriString(Uri.Path(s"api/job$subpath"))  // Default without trailing slash: query = Uri.Query("return" → "JobChainDetailed")))
    }
  }

  object processClass {
    def view[V <: ProcessClassView: ProcessClassView.Companion](processClassPath: ProcessClassPath): String = {
      val subpath = PathQueryHttp.toUriPath(PathQuery(processClassPath))
      uriString(Uri.Path(s"api/processClass$subpath"), "return" → implicitly[ProcessClassView.Companion[V]].name)
    }

    def views[V <: ProcessClassView: ProcessClassView.Companion](): String =
      uriString(Uri.Path(s"api/processClass/"), "return" → implicitly[ProcessClassView.Companion[V]].name)
//      val subpath = ProcessClassQueryHttp.toUriPath(query)
//      require(!subpath.endsWith("/"), "Invalid ProcessClassPath has trailing slash")
//      uriString(Uri(path = Uri.Path(s"api/processClass$subpath")))  // Default with trailing slash: query = Uri.Query("return" → "ProcessClassOverview")))
//    }
  }

  object task {
    def overview(taskId: TaskId) = uriString(Uri.Path("api/task") / taskId.string)
  }

  object agent {
    def agentUris = uriString(Uri.Path("api/agent/"))

    def forward(agentUri: String) = uriString(Uri.Path(s"api/agent/$agentUri"))
  }

  def events =  uriString(Uri.Path("api/event/"))

  def events(after: EventId = EventId.BeforeFirst, limit: Int = Int.MaxValue, reverse: Boolean = false, returnType: String = DefaultEventName) = {
    require(limit > 0, "Limit must not be below zero")
    val lim = if (reverse) -limit else limit
    uriString(Uri.Path("api/event/"),
      (returnType != DefaultEventName list ("return" → returnType)) :::
      (after != EventId.BeforeFirst list ("after" → s"$after")) :::
      (lim != Int.MaxValue list ("limit" → s"$lim")): _*)
  }

  def uriString(path: Uri.Path, parameters: (String, String)*): String =
    resolvePathUri(Uri(path = path, query = Uri.Query(parameters: _*))).toString

  def uriString(relativeUri: Uri): String = resolvePathUri(relativeUri).toString

  def /(relativeUri: Uri): Uri = resolvePathUri(relativeUri)

  private def resolvePathUri(relativeUri: Uri): Uri = resolveUri(relativeUri, rootUri)

  override def toString = s"SchedulerUris($schedulerUriString)"
}

object SchedulerUris {
  private val Prefix = "jobscheduler/master"
  val DefaultEventName = "Event"

  def apply(schedulerUri: Uri): SchedulerUris = apply(schedulerUri.toString)

  def apply(schedulerUri: String) = new SchedulerUris(schedulerUri stripSuffix "/")

  private[web] def resolveUri(uri: Uri, against: Uri) = {
    val scheme = emptyToNone(uri.scheme) getOrElse against.scheme
    val dummyAbsolute = Uri("dummy:")
    uri.resolvedAgainst(against resolvedAgainst dummyAbsolute).withScheme(scheme)
  }
}
