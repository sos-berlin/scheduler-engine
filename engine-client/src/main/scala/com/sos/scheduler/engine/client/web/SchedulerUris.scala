package com.sos.scheduler.engine.client.web

import com.sos.scheduler.engine.base.utils.ScalazStyle.OptionRichBoolean
import com.sos.scheduler.engine.client.web.SchedulerUris._
import com.sos.scheduler.engine.client.web.jobchain.{JobChainQueryHttp, PathQueryHttp}
import com.sos.scheduler.engine.common.scalautil.Collections._
import com.sos.scheduler.engine.data.event.EventId
import com.sos.scheduler.engine.data.job.{JobPath, TaskId}
import com.sos.scheduler.engine.data.jobchain.JobChainPath
import com.sos.scheduler.engine.data.order.OrderView
import com.sos.scheduler.engine.data.processclass.ProcessClassPath
import com.sos.scheduler.engine.data.queries.{JobChainQuery, OrderQuery, PathQuery}
import spray.http.Uri

/**
  * @author Joacim Zschimmer
  */
final class SchedulerUris private(schedulerUriString: String) {

  private val rootUri = Uri(s"$schedulerUriString/$Prefix/")

  lazy val command = resolvePathUri("api/command")

  lazy val overview = uriString("api")

  object order {
    def overviews(query: OrderQuery = OrderQuery.All): String =
      apply(query, Some("OrderOverview"))

    def treeComplemented[V <: OrderView: OrderView.Companion]: String = treeComplemented[V](OrderQuery.All)

    def treeComplemented[V <: OrderView: OrderView.Companion](query: OrderQuery): String = {
      val view = implicitly[OrderView.Companion[V]].name
      apply(query, Some(s"OrderTreeComplemented/$view"))
    }

    def complemented[V <: OrderView: OrderView.Companion](query: OrderQuery = OrderQuery.All): String = {
      val view = implicitly[OrderView.Companion[V]].name
      apply(query, Some(s"OrdersComplemented/$view"))
    }

    def apply[V <: OrderView: OrderView.Companion](query: OrderQuery): String = orders[V](query)

    def apply(query: OrderQuery, returnType: Option[String]): String = orders(query, returnType)

    def orders[V <: OrderView: OrderView.Companion](query: OrderQuery): String = {
      val view = implicitly[OrderView.Companion[V]]
      orders(query, returnType = Some(view.name))
    }

    def orders(query: OrderQuery, returnType: Option[String]): String = {
      val subpath = PathQueryHttp.toUriPath(query.jobChainPathQuery)
      uriString(Uri(
        path = Uri.Path(s"api/order$subpath"),
        query = Uri.Query(query.withoutPathToMap ++ (returnType map { o ⇒ "return" → o }))))
    }
  }

  object jobChain {
    def overviews(query: JobChainQuery = JobChainQuery.All): String = {
      val subpath = JobChainQueryHttp.toUriPath(query)
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
    def overviews(): String = uriString(Uri.Path(s"api/processClass/"))
//    def overviews(query: ProcessClassQuery = ProcessClassQuery.All): String = {
//      val subpath = ProcessClassQueryHttp.toUriPath(query)
//      require(subpath endsWith "/", "ProcessClassQuery must denote folder, terminated by a slash")
//      uriString(Uri(path = Uri.Path(s"api/processClass$subpath")))  // Default with trailing slash: query = Uri.Query("return" → "ProcessClassOverview")))
//    }

    def overview(processClassPath: ProcessClassPath): String = {
      val subpath = PathQueryHttp.toUriPath(PathQuery(processClassPath))
      require(!subpath.endsWith("/"), "Invalid ProcessClassPath has trailing slash")
      uriString(Uri.Path(s"api/processClass$subpath"), "return" → "ProcessClassOverview")
    }

    def details(processClassPath: ProcessClassPath): String = {
      val subpath = PathQueryHttp.toUriPath(PathQuery(processClassPath))
      require(!subpath.endsWith("/"), "Invalid ProcessClassPath has trailing slash")
      uriString(Uri.Path(s"api/processClass$subpath"))  // Default without trailing slash: query = Uri.Query("return" → "ProcessClassDetailed")))
    }
  }

  object task {
    def overview(taskId: TaskId) = uriString(Uri.Path("api/task") / taskId.string)
  }

  def events =  uriString(Uri.Path("api/event/"))

  def events(after: EventId = EventId.BeforeFirst, limit: Int = Int.MaxValue, reverse: Boolean = false) = {
    require(limit > 0, "Limit must not be below zero")
    val lim = if (reverse) -limit else limit
    uriString(Uri.Path("api/event/"),
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

  def apply(schedulerUri: Uri): SchedulerUris = apply(schedulerUri.toString)

  def apply(schedulerUri: String) = new SchedulerUris(schedulerUri stripSuffix "/")

  private[web] def resolveUri(uri: Uri, against: Uri) = {
    val scheme = emptyToNone(uri.scheme) getOrElse against.scheme
    val dummyAbsolute = Uri("dummy:")
    uri.resolvedAgainst(against resolvedAgainst dummyAbsolute).withScheme(scheme)
  }
}
