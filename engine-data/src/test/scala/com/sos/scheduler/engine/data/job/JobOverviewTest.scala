package com.sos.scheduler.engine.data.job

import com.sos.scheduler.engine.base.sprayjson.SprayJson._
import com.sos.scheduler.engine.data.filebased.FileBasedState
import com.sos.scheduler.engine.data.processclass.ProcessClassPath
import java.time.Instant
import org.junit.runner.RunWith
import org.scalatest.FreeSpec
import org.scalatest.junit.JUnitRunner
import spray.json._

/**
  * @author Joacim Zschimmer
  */
@RunWith(classOf[JUnitRunner])
final class JobOverviewTest extends FreeSpec
{
  "JSON" in {
    check(
      JobOverview(JobPath("/JOB"), FileBasedState.active, defaultProcessClassPath = Some(ProcessClassPath("/PROCESS-CLASS")),
        isOrderJob = true, title = "TITLE", enabled = true,
        JobState.running, stateText = "STATE TEXT", isInPeriod = true, nextStartTime = Some(Instant.parse("2018-10-01T11:11:11Z")),
        taskLimit = 10, usedTaskCount = 1, queuedTaskCount = 3, lateTaskCount = 2,
        obstacles = Set(JobObstacle.Stopped),
        taskObstacles = Map(
          TaskId(4711) → Set(TaskObstacle.WaitingForProcessClass)),
        Some(JobOverview.Error("ERROR-CODE", "ERROR MESSAGE"))),  // (No real combination)
      json"""{
        "path": "/JOB",
        "fileBasedState": "active",
        "defaultProcessClassPath": "/PROCESS-CLASS",
        "isOrderJob": true,
        "title": "TITLE",
        "enabled": true,
        "state": "running",
        "stateText": "STATE TEXT",
        "isInPeriod": true,
        "nextStartTime": "2018-10-01T11:11:11Z",
        "taskLimit": 10,
        "usedTaskCount": 1,
        "queuedTaskCount": 3,
        "lateTaskCount": 2,
        "obstacles": [
          { "TYPE": "Stopped" }
        ],
        "taskObstacles": {
          "4711": [
            { "TYPE": "WaitingForProcessClass" }
          ]
        },
        "error": {
          "code": "ERROR-CODE",
          "message": "ERROR MESSAGE"
        }
      }""")
  }

  private def check(q: JobOverview, json: JsValue) = {
    assert(q.toJson == json)
    assert(json.convertTo[JobOverview] == q)
  }
}
