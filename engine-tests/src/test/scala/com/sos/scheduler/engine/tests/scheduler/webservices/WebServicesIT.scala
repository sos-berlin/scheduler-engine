package com.sos.scheduler.engine.tests.scheduler.webservices

import com.sos.scheduler.engine.base.sprayjson.SprayJson._
import com.sos.scheduler.engine.client.web.StandardWebSchedulerClient
import com.sos.scheduler.engine.common.scalautil.Closers.implicits.RichClosersAutoCloseable
import com.sos.scheduler.engine.common.scalautil.Closers.withCloser
import com.sos.scheduler.engine.common.scalautil.Futures.implicits.SuccessFuture
import com.sos.scheduler.engine.common.scalautil.Logger
import com.sos.scheduler.engine.common.time.ScalaTime._
import com.sos.scheduler.engine.common.time.Stopwatch
import com.sos.scheduler.engine.common.utils.FreeTcpPortFinder.findRandomFreeTcpPort
import com.sos.scheduler.engine.data.event.{KeyedEvent, Snapshot}
import com.sos.scheduler.engine.data.filebased.FileBasedActivated
import com.sos.scheduler.engine.data.job.{JobPath, TaskClosed, TaskKey, TaskStarted}
import com.sos.scheduler.engine.data.jobchain.JobChainPath
import com.sos.scheduler.engine.data.xmlcommands.StartJobCommand
import com.sos.scheduler.engine.test.EventBusTestFutures.implicits.RichEventBus
import com.sos.scheduler.engine.test.SchedulerTestUtils.{startJob, startOrder}
import com.sos.scheduler.engine.test.configuration.TestConfiguration
import com.sos.scheduler.engine.test.scalatest.ScalaSchedulerTest
import com.sos.scheduler.engine.tests.scheduler.webservices.WebServicesIT._
import java.time.Instant.now
import java.time.temporal.ChronoField.MILLI_OF_SECOND
import org.junit.runner.RunWith
import org.scalatest.FreeSpec
import org.scalatest.junit.JUnitRunner
import scala.collection.mutable
import scala.concurrent.Promise
import spray.http.StatusCodes.BadRequest
import spray.httpx.SprayJsonSupport._
import spray.json.DefaultJsonProtocol._
import spray.json._

/**
  * JS-1793 Web services for Jobs.
  * @author Joacim Zschimmer
  */
@RunWith(classOf[JUnitRunner])
final class WebServicesIT extends FreeSpec with ScalaSchedulerTest
{
  private lazy val httpPort = findRandomFreeTcpPort()
  protected override lazy val testConfiguration = TestConfiguration(getClass,
    mainArguments = s"-http-port=$httpPort" :: "-log-level=warn" :: Nil)
  protected lazy val client = new StandardWebSchedulerClient(s"http://127.0.0.1:$httpPort").closeWithCloser
  private val enqueuedTaskStartAt = (now + 100.h).`with`(MILLI_OF_SECOND, 0)
  private val whenOrderTaskClosed = Promise[TaskClosed.type]()

  "/api/job/someFolder/test, running" in {
    scheduler executeXml StartJobCommand(TestJobPath, at = Some(StartJobCommand.At(enqueuedTaskStartAt)))
    eventBus.awaitingWhen[TaskStarted.type](_.key.jobPath == TestJobPath) {
      startOrder(TestJobChainPath orderKey "TEST-ORDER")
    }
    eventBus.on[TaskClosed.type] {
      case KeyedEvent(TaskKey(TestJobPath, _), event) ⇒
        whenOrderTaskClosed.trySuccess(event)
    }
    val jsObject = client.getByUri[JsObject]("api/job/someFolder/test") await TestTimeout
    val enqueuedAt = jsObject.fields("queuedTasks").asInstanceOf[JsArray].elements(0).asJsObject.fields("enqueuedAt").asInstanceOf[JsString].value
    val pid = jsObject.fields("runningTasks").asInstanceOf[JsArray].elements(0).asJsObject.fields("pid").asInstanceOf[JsNumber].value
    val startAt   = jsObject.fields("runningTasks").asInstanceOf[JsArray].elements(0).asJsObject.fields("startAt").asInstanceOf[JsString].value
    val startedAt = jsObject.fields("runningTasks").asInstanceOf[JsArray].elements(0).asJsObject.fields("startedAt").asInstanceOf[JsString].value
    val stepCount = jsObject.fields("runningTasks").asInstanceOf[JsArray].elements(0).asJsObject.fields("stepCount").asInstanceOf[JsNumber].value
    assert(jsObject.fields contains "eventId")
    assert(jsObject.copy(fields = jsObject.fields - "eventId") ==
      json"""{
        "overview": {
          "path": "/someFolder/test",
          "fileBasedState": "active",
          "isOrderJob": false,
          "title": "JOB TITLE",
          "state": "running",
          "stateText": "",
          "enabled": true,
          "isInPeriod": true,
          "usedTaskCount": 1,
          "queuedTaskCount": 1,
          "lateTaskCount": 0,
          "taskLimit": 1,
          "obstacles": [
            {
              "TYPE": "TaskLimitReached",
              "limit": 1
            }
          ],
          "taskObstacles": {}
        },
        "defaultParameters": {
          "JOB-PARAM": "JOB-VALUE"
        } ,
        "queuedTasks": [
          {
            "taskId": "3",
            "enqueuedAt": "$enqueuedAt",
            "startAt": "${enqueuedTaskStartAt.toString}"
          }
        ],
        "runningTasks": [
          {
            "taskId": "4",
            "cause": "order",
            "pid": $pid,
            "startAt": "$startAt",
            "startedAt": "$startedAt",
            "stepCount": $stepCount,
            "order": {
              "orderId": "TEST-ORDER",
              "jobChainPath": "/someFolder/test-jobchain",
              "nodeId": "100"
            }
          }
        ]
      }""")
  }

  "/api/job/someFolder/?return=JocOrderStatistics&isDistributed=false" in {
    val jsObject = client.getByUri[JsObject]("api/job/someFolder/?return=JocOrderStatistics&isDistributed=false") await TestTimeout
    assert(jsObject.fields("/someFolder/test") ==
      json"""{
        "total": 1,
        "notPlanned": 0,
        "planned": 0,
        "due": 0,
        "started": 1,
        "inTask": 1,
        "inTaskProcess": 1,
        "occupiedByClusterMember": 0,
        "setback": 0,
        "waitingForResource": 0,
        "suspended": 0,
        "blacklisted": 0,
        "permanent": 0,
        "fileOrder": 0
      }""")
  }

  "/api/job/someFolder/?return=JocOrderStatistics is rejected" in {
    val response = intercept[spray.httpx.UnsuccessfulResponseException] {
      client.getByUri[JsObject]("api/job/someFolder/?return=JocOrderStatistics") await TestTimeout
    }.response
    assert(response.status == BadRequest && response.entity.asString == "return=JocOrderStatistics requires isDistributed=false")
  }

  "/api/job/someFolder/" - {
    "state=running" in {
      val snapshot = client.getByUri[Snapshot[JsArray]]("api/job/someFolder/?state=running") await TestTimeout
      assert(snapshot.value.elements.size == 1)
    }

    "state=pending,running" in {
      val snapshot = client.getByUri[Snapshot[JsArray]]("api/job/someFolder/?state=running") await TestTimeout
      assert(snapshot.value.elements.size == 1)
    }

    "state=stopped" in {
      val snapshot = client.getByUri[Snapshot[JsArray]]("api/job/someFolder/?state=stopped") await TestTimeout
      assert(snapshot.value.elements.size == 0)
    }

    "one job" in {
      whenOrderTaskClosed.future await TestTimeout
      val snapshot = client.getByUri[Snapshot[JsArray]]("api/job/someFolder/") await TestTimeout
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
          "queuedTaskCount": 1,
          "lateTaskCount": 0,
          "taskLimit": 1,
          "obstacles": [],
          "taskObstacles": {}
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

  "WaitingForProcessClass" in {
    scheduler executeXml <job name="waiting-for-process" title="JOB TITLE" process_class="/test"><script language="shell">exit</script></job>
    val taskId = startJob(JobPath("/waiting-for-process")).taskId
    sleep(3.s)  // Let JobSchedule start the task
    val jsObject = client.getByUri[JsObject]("api/job/waiting-for-process?return=JobOverview") await TestTimeout
    logger.info(scheduler.executeXml(<show_task id={taskId.number.toString}/>).string)
    assert(jsObject.copy(fields = jsObject.fields - "eventId") ==
      json"""{
        "path": "/waiting-for-process",
        "fileBasedState": "active",
        "isOrderJob": false,
        "title": "JOB TITLE",
        "defaultProcessClassPath": "/test",
        "state": "running",
        "stateText": "",
        "enabled": true,
        "isInPeriod": true,
        "usedTaskCount": 1,
        "queuedTaskCount": 0,
        "lateTaskCount": 0,
        "taskLimit": 1,
        "obstacles": [
          {
            "limit": 1,
            "TYPE": "TaskLimitReached"
          }
        ],
        "taskObstacles": {
          "${taskId.number.toString}": [
            { "TYPE": "WaitingForAgent" }
          ]
        }
      }""")
  }
}

object WebServicesIT {
  private val logger = Logger(getClass)
  private val TestJobPath = JobPath("/someFolder/test")
  private val TestJobChainPath = JobChainPath("/someFolder/test-jobchain")
}
