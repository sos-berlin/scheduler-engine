package com.sos.scheduler.engine.client.web

import com.sos.scheduler.engine.common.time.ScalaTime._
import com.sos.scheduler.engine.data.event.{Event, EventId, EventRequest, ReverseEventRequest}
import com.sos.scheduler.engine.data.filebased.FileBasedEvent
import com.sos.scheduler.engine.data.folder.FolderPath
import com.sos.scheduler.engine.data.job.{JobDescription, JobEvent, JobOverview, JobPath, TaskEvent, TaskId}
import com.sos.scheduler.engine.data.jobchain.JobChainPath
import com.sos.scheduler.engine.data.order.{JobChainEvent, OrderDetailed, OrderOverview, OrderSourceType}
import com.sos.scheduler.engine.data.processclass.{ProcessClassDetailed, ProcessClassOverview, ProcessClassPath}
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

  "/jobscheduler" in {
    assert(SchedulerUris("http://0.0.0.0:1111/jobscheduler").masterUri == uris.masterUri)
    assert(SchedulerUris("http://0.0.0.0:1111/xxxx").masterUri == Uri("http://0.0.0.0:1111/xxxx/master/"))
  }

  "overview" in {
    assert(uris.overview == "http://0.0.0.0:1111/jobscheduler/master/api")
  }

  "fileBasedDetailed" in {
    assert(uris.anyTypeFileBaseds(FolderPath("/FOLDER"), "FileBasedDetailed") ==
      "http://0.0.0.0:1111/jobscheduler/master/api/fileBased/FOLDER/?return=FileBasedDetailed")
    assert(uris.fileBaseds[JobChainPath](FolderPath("/FOLDER"), "FileBasedDetailed") ==
      "http://0.0.0.0:1111/jobscheduler/master/api/jobChain/FOLDER/?return=FileBasedDetailed")
    assert(uris.fileBased(JobChainPath("/FOLDER/TEST") orderKey "1", "FileBasedDetailed") ==
      "http://0.0.0.0:1111/jobscheduler/master/api/order/FOLDER/TEST,1?return=FileBasedDetailed")
  }

  "order" - {
    "GET OrderOverview" in {
      assert(uris.order[OrderOverview](OrderQuery.All) ==
        "http://0.0.0.0:1111/jobscheduler/master/api/order/?return=OrderOverview")
    }

    "POST OrderOverview" in {
      assert(uris.order.forPost[OrderOverview] ==
        "http://0.0.0.0:1111/jobscheduler/master/api/order?return=OrderOverview")
    }

    "GET OrderDetailed" in {
      assert(uris.order[OrderDetailed](OrderQuery.All) ==
        "http://0.0.0.0:1111/jobscheduler/master/api/order/?return=OrderDetailed")
    }

    "POST OrderDetailed" in {
      assert(uris.order.forPost[OrderDetailed] ==
        "http://0.0.0.0:1111/jobscheduler/master/api/order?return=OrderDetailed")
    }

    "GET OrderOverview isSuspended" in {
      assert(
        uris.order[OrderOverview](OrderQuery(isSuspended = Some(true))) ==
        Uri("http://0.0.0.0:1111/jobscheduler/master/api/order/").withQuery(Uri.Query(
          "isSuspended" → "true",
          "return" → "OrderOverview")).toString)
    }

    "GET OrderOverview isSuspended isBlacklisted FileOrder AdHoc" in {
      assert(uris.order[OrderOverview](
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
    }

    "GET OrderDetails isSuspended OrderKey" in {
      assert(uris.order[OrderDetailed](OrderQuery(isSuspended = Some(true)).withOrderKey(JobChainPath("/FOLDER/TEST") orderKey "ID/1-Ä")) ==
        Uri("http://0.0.0.0:1111/jobscheduler/master/api/order/FOLDER/TEST")
          .withQuery(Uri.Query(
            "orderIds" → "ID/1-Ä",
            "isSuspended" → "true",
            "return" → "OrderDetailed")).toString)
    }

    "GET with umlauts in OrderId" in {
      assert(uris.order(OrderQuery.All.withOrderKey(JobChainPath("/FOLDER/TEST") orderKey "ID/1:Ä?"), returnType = None) ==
        "http://0.0.0.0:1111/jobscheduler/master/api/order/FOLDER/TEST?orderIds=ID/1:%C3%84?")
    }
  }

  "order.ordersComplemented" - {
    "GET OrderOverview" in {
      assert(uris.order.complemented[OrderOverview](OrderQuery.All) ==
        "http://0.0.0.0:1111/jobscheduler/master/api/order/?return=OrdersComplemented/OrderOverview")
    }

    "POST OrderOverview" in {
      assert(uris.order.complementedForPost[OrderOverview] ==
        "http://0.0.0.0:1111/jobscheduler/master/api/order?return=OrdersComplemented/OrderOverview")
    }

    "GET OrderDetailed" in {
      assert(uris.order.complemented[OrderDetailed](OrderQuery.All) ==
        "http://0.0.0.0:1111/jobscheduler/master/api/order/?return=OrdersComplemented/OrderDetailed")
    }

    "POST OrderDetailed" in {
      assert(uris.order.complementedForPost[OrderDetailed] ==
        "http://0.0.0.0:1111/jobscheduler/master/api/order?return=OrdersComplemented/OrderDetailed")
    }

    "GET OrderOverview isSuspended" in {
      assert(
        uris.order.complemented[OrderOverview](OrderQuery(isSuspended = Some(true))) ==
        Uri("http://0.0.0.0:1111/jobscheduler/master/api/order/").withQuery(Uri.Query(
          "isSuspended" → "true",
          "return" → "OrdersComplemented/OrderOverview")).toString)
    }
  }

  "order.orderTreeComplemented" - {
    "GET OrderOverview" in {
      assert(uris.order.treeComplemented[OrderOverview](OrderQuery.All) ==
        "http://0.0.0.0:1111/jobscheduler/master/api/order/?return=OrderTreeComplemented/OrderOverview")
    }

    "POST OrderOverview" in {
      assert(uris.order.treeComplementedForPost[OrderOverview] ==
        "http://0.0.0.0:1111/jobscheduler/master/api/order?return=OrderTreeComplemented/OrderOverview")
    }

    "GET OrderDetailed" in {
      assert(uris.order.treeComplemented[OrderDetailed](OrderQuery.All) ==
        "http://0.0.0.0:1111/jobscheduler/master/api/order/?return=OrderTreeComplemented/OrderDetailed")
    }

    "POST OrderDetailed" in {
      assert(uris.order.treeComplementedForPost[OrderDetailed] ==
        "http://0.0.0.0:1111/jobscheduler/master/api/order?return=OrderTreeComplemented/OrderDetailed")
    }

    "GET OrderOverview isSuspended" in {
      assert(
        uris.order.treeComplemented[OrderOverview](OrderQuery(isSuspended = Some(true))) ==
        Uri("http://0.0.0.0:1111/jobscheduler/master/api/order/").withQuery(Uri.Query(
          "isSuspended" → "true",
          "return" → "OrderTreeComplemented/OrderOverview")).toString)
    }
  }

  "jobChain" - {
    "overviews" in {
      assert(uris.jobChain.overviews(JobChainQuery(PathQuery[JobChainPath]("/a/"))) == "http://0.0.0.0:1111/jobscheduler/master/api/jobChain/a/")
      intercept[IllegalArgumentException] { uris.jobChain.overviews(JobChainQuery(PathQuery[JobChainPath]("/a"))) }
    }

    "overview" in {
      assert(uris.jobChain.overview(JobChainPath("/a/b")) == "http://0.0.0.0:1111/jobscheduler/master/api/jobChain/a/b?return=JobChainOverview")
    }

    "detail" in {
      assert(uris.jobChain.detailed(JobChainPath("/a/b")) == "http://0.0.0.0:1111/jobscheduler/master/api/jobChain/a/b")
    }

    "events" in {
      assert(uris.jobChain.events(JobChainPath("/a/b"), EventRequest.singleClass[JobChainEvent](after = 7, 1.s)) ==
        "http://0.0.0.0:1111/jobscheduler/master/api/jobChain/a/b?return=JobChainEvent&timeout=PT1S&after=7")
    }
  }

  "job" - {
    "JobOverview" in {
      assert(uris.job[JobOverview](FolderPath("/a")) == "http://0.0.0.0:1111/jobscheduler/master/api/job/a/?return=JobOverview")
    }

    "JobDescription" in {
      assert(uris.job[JobDescription](JobPath("/a/b")) == "http://0.0.0.0:1111/jobscheduler/master/api/job/a/b?return=JobDescription")
    }

    "events" in {
      assert(uris.job.events(PathQuery(JobPath("/a/b")), EventRequest.singleClass[JobEvent](after = 7, 1.s)) ==
        "http://0.0.0.0:1111/jobscheduler/master/api/job/a/b?return=JobEvent&timeout=PT1S&after=7")
    }
  }

  "task" - {
    "overview" in {
      assert(uris.task.overview(TaskId(123)) == "http://0.0.0.0:1111/jobscheduler/master/api/task/123")
    }

    "events" in {
      assert(uris.task.events(TaskId(123), EventRequest.singleClass[TaskEvent](after = 7, 1.s)) ==
        "http://0.0.0.0:1111/jobscheduler/master/api/task?taskId=123&return=TaskEvent&timeout=PT1S&after=7")
    }

    "eventsBy" in {
      assert(uris.task.eventsBy(PathQuery(JobPath("/folder/job")), EventRequest.singleClass[TaskEvent](after = 7, 1.s)) ==
        "http://0.0.0.0:1111/jobscheduler/master/api/task/folder/job?return=TaskEvent&timeout=PT1S&after=7")
    }
  }

  "processClass" - {
    "Single ProcessClassOverview" in {
      assert(uris.processClass.view[ProcessClassOverview](ProcessClassPath("/TEST")) ==
        "http://0.0.0.0:1111/jobscheduler/master/api/processClass/TEST?return=ProcessClassOverview")
    }

    "Single ProcessClassDetailed" in {
      assert(uris.processClass.view[ProcessClassDetailed](ProcessClassPath("/TEST")) ==
        "http://0.0.0.0:1111/jobscheduler/master/api/processClass/TEST?return=ProcessClassDetailed")
    }

    "Multiple ProcessClassOverview" in {
      assert(uris.processClass.views[ProcessClassOverview](PathQuery(FolderPath("/FOLDER"))) ==
        "http://0.0.0.0:1111/jobscheduler/master/api/processClass/FOLDER/?return=ProcessClassOverview")
    }

    "Multiple ProcessClassDetailed" in {
      assert(uris.processClass.views[ProcessClassDetailed](PathQuery.All) ==
        "http://0.0.0.0:1111/jobscheduler/master/api/processClass/?return=ProcessClassDetailed")
    }
  }

  "event" in {
    assert(uris.events(EventRequest.singleClass[Event](timeout = 123456.ms, after = 111222333444555666L)) ==
      "http://0.0.0.0:1111/jobscheduler/master/api/event?timeout=PT2M3.456S&after=111222333444555666")
    assert(uris.events(EventRequest.singleClass[Event](after = EventId(7), timeout = 1.s)) ==
      "http://0.0.0.0:1111/jobscheduler/master/api/event?timeout=PT1S&after=7")
    assert(uris.events(EventRequest.singleClass[Event](limit = 100, timeout = 100.ms, after = EventId.BeforeFirst)) ==
      "http://0.0.0.0:1111/jobscheduler/master/api/event?timeout=PT0.1S&limit=100&after=0")
    assert(uris.events(ReverseEventRequest[FileBasedEvent](after = EventId(7), limit = 100)) ==
      "http://0.0.0.0:1111/jobscheduler/master/api/event?return=FileBasedEvent&limit=-100&after=7")
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
