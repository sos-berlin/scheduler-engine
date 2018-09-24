package com.sos.scheduler.engine.tests.scheduler.webservices

import com.sos.scheduler.engine.base.sprayjson.SprayJson._
import com.sos.scheduler.engine.client.web.StandardWebSchedulerClient
import com.sos.scheduler.engine.common.scalautil.Closers.implicits.RichClosersAutoCloseable
import com.sos.scheduler.engine.common.scalautil.Closers.withCloser
import com.sos.scheduler.engine.common.scalautil.Futures.implicits.SuccessFuture
import com.sos.scheduler.engine.common.scalautil.{Closers, Logger}
import com.sos.scheduler.engine.common.time.Stopwatch
import com.sos.scheduler.engine.common.utils.FreeTcpPortFinder.findRandomFreeTcpPort
import com.sos.scheduler.engine.data.event.{KeyedEvent, Snapshot}
import com.sos.scheduler.engine.data.filebased.FileBasedActivated
import com.sos.scheduler.engine.data.job.{JobPath, TaskStarted}
import com.sos.scheduler.engine.test.EventBusTestFutures.implicits.RichEventBus
import com.sos.scheduler.engine.test.SchedulerTestUtils.{TaskRun, startJob}
import com.sos.scheduler.engine.test.configuration.TestConfiguration
import com.sos.scheduler.engine.test.scalatest.ScalaSchedulerTest
import com.sos.scheduler.engine.tests.scheduler.webservices.WebServicesIT._
import org.junit.runner.RunWith
import org.scalatest.FreeSpec
import org.scalatest.junit.JUnitRunner
import scala.collection.mutable
import spray.httpx.SprayJsonSupport._
import spray.json.DefaultJsonProtocol._
import spray.json._

/**
  * JS_1793 Web services for Jobs.
  * @author Joacim Zschimmer
  */
@RunWith(classOf[JUnitRunner])
final class WebServicesIT extends FreeSpec with ScalaSchedulerTest
{
  private lazy val httpPort = findRandomFreeTcpPort()
  protected override lazy val testConfiguration = TestConfiguration(getClass,
    mainArguments = s"-http-port=$httpPort" :: "-log-level=warn" :: Nil)
  protected lazy val client = new StandardWebSchedulerClient(s"http://127.0.0.1:$httpPort").closeWithCloser

  "/api/job/test, running" in {
    var run: TaskRun = null
    eventBus.awaitingWhen[TaskStarted.type](_.key.jobPath == TestJobPath) {
      run = startJob(TestJobPath)
    }
    val jsObject = client.getByUri[JsObject]("api/job/someFolder/test").await(TestTimeout)
    assert(jsObject.copy(fields = jsObject.fields - "eventId") ==
      json"""{
        "path": "/someFolder/test",
        "fileBasedState": "active",
        "isOrderJob": false,
        "title": "JOB TITLE",
        "state": "running",
        "stateText": "",
        "enabled": true,
        "isInPeriod": true,
        "usedTaskCount": 1,
        "taskQueueLength": 0,
        "taskLimit": 1,
        "obstacles": [
          {
            "TYPE": "TaskLimitReached",
            "limit": 1
          }
        ]
      }""")
    run.closed await TestTimeout
  }

  "/api/job/someFolder/" - {
    "one job" in {
      val snapshot = client.getByUri[Snapshot[JsArray]]("api/job/someFolder/").await(TestTimeout)
      val jsObject = snapshot.value.elements.head.asJsObject
      assert(jsObject ==
        json"""{
          "path": "/someFolder/test",
          "fileBasedState": "active",
          "isOrderJob": false,
          "title": "JOB TITLE",
          "enabled": true,
          "state": "pending",
          "stateText": "",
          "isInPeriod": true,
          "usedTaskCount": 0,
          "taskQueueLength": 0,
          "taskLimit": 1,
          "obstacles": []
        }""")
    }

    val n = 1000
    s"$n jobs" in {
      val logs = mutable.Buffer[String]()

      withCloser { implicit closer ⇒
        var activatedJobCount = 0
        eventBus.on[FileBasedActivated.type] {
          case KeyedEvent(_: JobPath, FileBasedActivated) ⇒ activatedJobCount += 1
        }
        val stopwatch = new Stopwatch
        controller.withEventPipe { eventPipe ⇒
          scheduler executeXml <add_jobs>{for (i ← 1 to n) yield <job name={s"test-$i"}><script language="shell">exit</script></job>}</add_jobs>
          for (_ ← 1 to n) eventPipe.nextAny[FileBasedActivated.type]
          logs += "<job>            " + stopwatch.itemsPerSecondString(n, "job")
        }
      }

      for (i ← 1 to 9) {
        val stopwatch = new Stopwatch
        val response = scheduler executeXml <show_jobs/>
        logs += s"<show_jobs> ($i): " + stopwatch.itemsPerSecondString(n, "job") + s", ${response.string.length / 1000}kB"
      }

      for (i ← 1 to 9) {
        val stopwatch = new Stopwatch
        val jsonString = client.getByUri[String]("api/job/").await(TestTimeout)
        logs += s"Web service ($i): " + stopwatch.itemsPerSecondString(n, "job") + s", ${jsonString.length / 1000}kB"
        if (i == 1) {
          val snapshot = jsonString.parseJson.convertTo[Snapshot[JsArray]]
          assert(snapshot.value.elements.length >= n)
        }
      }

      for (line ← logs) logger.info(line)
    }
  }
}

object WebServicesIT {
  private val logger = Logger(getClass)
  private val TestJobPath = JobPath("/someFolder/test")
}
