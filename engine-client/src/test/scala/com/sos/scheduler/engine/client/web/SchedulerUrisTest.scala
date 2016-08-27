package com.sos.scheduler.engine.client.web

import com.sos.scheduler.engine.data.event.EventId
import com.sos.scheduler.engine.data.job.{JobPath, TaskId}
import com.sos.scheduler.engine.data.jobchain.JobChainPath
import com.sos.scheduler.engine.data.order.{OrderDetailed, OrderOverview, OrderSourceType}
import com.sos.scheduler.engine.data.queries.{JobChainQuery, OrderQuery, PathQuery}
import org.junit.runner.RunWith
import org.scalatest.FreeSpec
import org.scalatest.junit.JUnitRunner
import spray.http.Uri

/**
  * @author Joacim Zschimmer
  */
@RunWith(classOf[JUnitRunner])
final class SchedulerUrisTest extends FreeSpec {

  private val uris = SchedulerUris("http://0.0.0.0:1111/")

  "overview" in {
    assert(uris.overview == "http://0.0.0.0:1111/jobscheduler/master/api")
  }

  "order.overview" in {
    assert(uris.order.overviews(OrderQuery.All) == "http://0.0.0.0:1111/jobscheduler/master/api/order/?return=OrderOverview")
    assert(uris.order.overviews(OrderQuery(isSuspended = Some(true))) ==
      Uri("http://0.0.0.0:1111/jobscheduler/master/api/order/").withQuery(Uri.Query(
        "isSuspended" → "true",
        "return" → "OrderOverview")).toString)
    assert(uris.order.overviews(OrderQuery(isSuspended = Some(true), isBlacklisted = Some(false))) ==
      Uri("http://0.0.0.0:1111/jobscheduler/master/api/order/").withQuery(Uri.Query(
        "isSuspended" → "true",
        "isBlacklisted" → "false",
        "return" → "OrderOverview")).toString)
  }

  "order.orders" in {
    assert(uris.order.orders[OrderOverview](OrderQuery.All) ==
      "http://0.0.0.0:1111/jobscheduler/master/api/order/?return=OrderOverview")
    assert(uris.order.orders[OrderDetailed](OrderQuery.All) ==
      "http://0.0.0.0:1111/jobscheduler/master/api/order/?return=OrderDetailed")
    assert(
      uris.order.orders[OrderOverview](OrderQuery(isSuspended = Some(true))) ==
      Uri("http://0.0.0.0:1111/jobscheduler/master/api/order/").withQuery(Uri.Query(
        "isSuspended" → "true",
        "return" → "OrderOverview")).toString)
    assert(uris.order.orders[OrderOverview](
      OrderQuery(
        isSuspended = Some(true),
        isBlacklisted = Some(false),
        isOrderSourceType = Some(Set(OrderSourceType.FileOrder, OrderSourceType.AdHoc)))) ==
      Uri("http://0.0.0.0:1111/jobscheduler/master/api/order/")
        .withQuery(Uri.Query(
          "isSuspended" → "true",
          "isBlacklisted" → "false",
          "isOrderSourceType" → "FileOrder,AdHoc",  // Incidentally, Scala Set with two elements retains orders
          "return" → "OrderOverview")).toString)
    assert(uris.order.orders[OrderDetailed](OrderQuery(isSuspended = Some(true)).withOrderKey(JobChainPath("/FOLDER/TEST") orderKey "ID/1-Ä")) ==
      Uri("http://0.0.0.0:1111/jobscheduler/master/api/order/FOLDER/TEST")
        .withQuery(Uri.Query(
          "orderId" → "ID/1-Ä",
          "isSuspended" → "true",
          "return" → "OrderDetailed")).toString)
    assert(uris.order.orders(OrderQuery.All.withOrderKey(JobChainPath("/FOLDER/TEST") orderKey "ID/1:Ä?"), returnType = None) ==
      "http://0.0.0.0:1111/jobscheduler/master/api/order/FOLDER/TEST?orderId=ID/1:%C3%84?")
  }

  "order.ordersComplemented" in {
    assert(uris.order.complemented[OrderOverview](OrderQuery.All) ==
      "http://0.0.0.0:1111/jobscheduler/master/api/order/?return=OrdersComplemented/OrderOverview")
    assert(uris.order.complemented[OrderDetailed](OrderQuery.All) ==
      "http://0.0.0.0:1111/jobscheduler/master/api/order/?return=OrdersComplemented/OrderDetailed")
    assert(
      uris.order.complemented[OrderOverview](OrderQuery(isSuspended = Some(true))) ==
      Uri("http://0.0.0.0:1111/jobscheduler/master/api/order/").withQuery(Uri.Query(
        "isSuspended" → "true",
        "return" → "OrdersComplemented/OrderOverview")).toString)
  }

  "order.orderTreeComplemented" in {
    assert(uris.order.treeComplemented[OrderOverview](OrderQuery.All) ==
      "http://0.0.0.0:1111/jobscheduler/master/api/order/?return=OrderTreeComplemented/OrderOverview")
    assert(uris.order.treeComplemented[OrderDetailed](OrderQuery.All) ==
      "http://0.0.0.0:1111/jobscheduler/master/api/order/?return=OrderTreeComplemented/OrderDetailed")
    assert(
      uris.order.treeComplemented[OrderOverview](OrderQuery(isSuspended = Some(true))) ==
      Uri("http://0.0.0.0:1111/jobscheduler/master/api/order/").withQuery(Uri.Query(
        "isSuspended" → "true",
        "return" → "OrderTreeComplemented/OrderOverview")).toString)
  }

  "jobChain.overviews" in {
    assert(uris.jobChain.overviews(JobChainQuery.Standard(PathQuery[JobChainPath]("/a/"))) == "http://0.0.0.0:1111/jobscheduler/master/api/jobChain/a/")
    intercept[IllegalArgumentException] { uris.jobChain.overviews(JobChainQuery.Standard(PathQuery[JobChainPath]("/a"))) }
  }

  "jobChain.overview" in {
    assert(uris.jobChain.overview(JobChainPath("/a/b")) == "http://0.0.0.0:1111/jobscheduler/master/api/jobChain/a/b?return=JobChainOverview")
  }

  "jobChain.detail" in {
    assert(uris.jobChain.details(JobChainPath("/a/b")) == "http://0.0.0.0:1111/jobscheduler/master/api/jobChain/a/b")
  }

  "job.overviews" in {
    assert(uris.job.overviews() == "http://0.0.0.0:1111/jobscheduler/master/api/job/")
    //intercept[IllegalArgumentException] { uris.job.overviews(JobQuery.Standard(PathQuery("/a"))) }
//    assert(uris.job.overviews(JobQuery.Standard(PathQuery("/a/"))) == "http://0.0.0.0:1111/jobscheduler/master/api/job/a/")
//    intercept[IllegalArgumentException] { uris.job.overviews(JobChainQuery.Standard(PathQuery("/a"))) }
  }

  "job.overview" in {
    assert(uris.job.overview(JobPath("/a/b")) == "http://0.0.0.0:1111/jobscheduler/master/api/job/a/b?return=JobOverview")
  }

  "job.detail" in {
    assert(uris.job.details(JobPath("/a/b")) == "http://0.0.0.0:1111/jobscheduler/master/api/job/a/b")
  }

  "task.overview" in {
    assert(uris.task.overview(TaskId(123)) == "http://0.0.0.0:1111/jobscheduler/master/api/task/123")
  }

  "event" in {
    assert(uris.events == "http://0.0.0.0:1111/jobscheduler/master/api/event/")
    assert(uris.events(after = EventId(7)) == "http://0.0.0.0:1111/jobscheduler/master/api/event/?after=7")
    assert(uris.events(limit = 100) == "http://0.0.0.0:1111/jobscheduler/master/api/event/?limit=100")
    assert(uris.events(limit = 100, reverse = true) == "http://0.0.0.0:1111/jobscheduler/master/api/event/?limit=-100")
  }

  "resolveUri" in {
    import SchedulerUris.resolveUri
    assert(resolveUri(Uri(""), Uri("/resolved")) == Uri("/resolved"))
    assert(resolveUri(Uri("path"), Uri("/resolved")) == Uri("/path"))
    assert(resolveUri(Uri("path"), Uri("/resolved/")) == Uri("/resolved/path"))
    assert(resolveUri(Uri("path"), Uri("resolved:")) == Uri("resolved:path"))
    assert(resolveUri(Uri("/path"), Uri("resolved:")) == Uri("resolved:///path"))
    assert(resolveUri(Uri("scheme:path"), Uri("resolved:/")) == Uri("scheme:path"))
    assert(resolveUri(Uri("scheme:/path"), Uri("resolved:/")) == Uri("scheme:/path"))
    assert(resolveUri(Uri("scheme://path"), Uri("resolved:/")) == Uri("scheme://path"))
    assert(resolveUri(Uri("scheme:///path"), Uri("resolved:/")) == Uri("scheme:///path"))
  }
}
