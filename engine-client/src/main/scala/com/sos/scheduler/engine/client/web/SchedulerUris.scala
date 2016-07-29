package com.sos.scheduler.engine.client.web

import com.sos.scheduler.engine.client.web.SchedulerUris._
import com.sos.scheduler.engine.client.web.jobchain.JobChainQueryHttp
import com.sos.scheduler.engine.client.web.order.OrderQueryHttp
import com.sos.scheduler.engine.common.scalautil.Collections._
import com.sos.scheduler.engine.data.job.TaskId
import com.sos.scheduler.engine.data.jobchain.{JobChainPath, JobChainQuery}
import com.sos.scheduler.engine.data.order.OrderQuery
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

    def treeComplemented: String = treeComplemented(OrderQuery.All)

    def treeComplemented(query: OrderQuery): String =
      apply(query, Some("OrderTreeComplemented"))

    def fullOverview(query: OrderQuery = OrderQuery.All): String =
      apply(query, Some("OrdersFullOverview"))

    def apply(orderQuery: OrderQuery, returnType: Option[String]): String = {
      val subpath = JobChainQueryHttp.toUriPath(orderQuery.jobChainQuery)
      resolvePathUri(Uri(
        path = Uri.Path(s"api/order$subpath"),
        query = Uri.Query(OrderQueryHttp.toHttpQueryMap(orderQuery) ++ (returnType map { o ⇒ "return" → o }))))
      .toString
    }
  }

  object jobChain {
    def overviews(query: JobChainQuery = JobChainQuery.All): String = {
      val subpath = JobChainQueryHttp.toUriPath(query)
      require(subpath endsWith "/", "JobChainQuery must denote folder, terminated by a slash")
      uriString(Uri(path = Uri.Path(s"api/jobChain$subpath")))  // Default with trailing slash: query = Uri.Query("return" → "JobChainOverview")))
    }

    def overview(jobChainPath: JobChainPath): String = {
      val subpath = JobChainQueryHttp.toUriPath(jobChainPath)
      require(!subpath.endsWith("/"), "Invalid JobChainPath has trailing slash")
      uriString(Uri(path = Uri.Path(s"api/jobChain$subpath"), query = Uri.Query("return" → "JobChainOverview")))
    }

    def details(jobChainPath: JobChainPath): String = {
      val subpath = JobChainQueryHttp.toUriPath(jobChainPath)
      require(!subpath.endsWith("/"), "Invalid JobChainPath has trailing slash")
      uriString(Uri(path = Uri.Path(s"api/jobChain$subpath")))  // Default without trailing slash: query = Uri.Query("return" → "JobChainDetails")))
    }
  }

  object task {
    def overview(taskId: TaskId) = uriString(Uri(path = Uri.Path("api/task") / taskId.string))
  }

  /**
    * Public for tests.
    */
  def uriString(uri: Uri): String = resolvePathUri(uri).toString

  def resolvePathUri(uri: Uri): Uri = resolveUri(uri, rootUri)

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
