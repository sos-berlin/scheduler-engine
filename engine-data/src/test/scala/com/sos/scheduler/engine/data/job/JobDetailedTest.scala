package com.sos.scheduler.engine.data.job

import com.sos.scheduler.engine.base.sprayjson.SprayJson._
import com.sos.scheduler.engine.data.filebased.FileBasedState
import com.sos.scheduler.engine.data.jobchain.{JobChainPath, NodeId}
import com.sos.scheduler.engine.data.order.OrderId
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
final class JobDetailedTest extends FreeSpec
{
  "JSON" in {
    check(
      JobDetailed(
        JobOverview(JobPath("/JOB"), FileBasedState.active, defaultProcessClassPath = Some(ProcessClassPath("/PROCESS-CLASS")),
          isOrderJob = true, title = "TITLE", enabled = true,
          JobState.running, stateText = "STATE TEXT", isInPeriod = true, nextStartTime = Some(Instant.parse("2018-10-01T11:11:11Z")),
          taskLimit = 10, usedTaskCount = 1, queuedTaskCount = 3, lateTaskCount = 2,
          obstacles = Set(JobObstacle.Stopped),
          taskObstacles = Map(
            TaskId(4711) → Set(TaskObstacle.WaitingForProcessClass)),
          Some(JobOverview.Error("ERROR-CODE", "ERROR MESSAGE"))),
        Map("PARAM" → "VALUE"),
        queuedTasks = Vector(
          JobDetailed.QueuedTask(TaskId(1000), Some(Instant.parse("2018-10-01T12:12:12Z")), Some(Instant.parse("2018-10-01T13:13:13Z")))),
        runningTasks = Vector(
          JobDetailed.RunningTask(TaskId(1001), "order",
            Some(Instant.parse("2018-10-01T12:12:12Z")), Some(Instant.parse("2018-10-01T13:13:13Z")), Instant.parse("2018-10-01T14:14:14Z"),
            Some(1234), 22,
            Some(JobDetailed.TaskOrder(OrderId("ORDER-ID"), JobChainPath("/JOB-CHAIN"), NodeId("100")))))),  // (No real combination)
      json"""{
        "overview": {
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
        },
        "defaultParameters": {
          "PARAM": "VALUE"
        },
        "queuedTasks": [
          {
            "taskId": "1000",
            "enqueuedAt": "2018-10-01T12:12:12Z",
            "startAt": "2018-10-01T13:13:13Z"
          }
        ],
        "runningTasks": [
          {
            "taskId": "1001",
            "pid": 1234,
            "cause": "order",
            "enqueuedAt": "2018-10-01T12:12:12Z",
            "startAt": "2018-10-01T13:13:13Z",
            "startedAt": "2018-10-01T14:14:14Z",
            "stepCount": 22,
            "order": {
              "orderId": "ORDER-ID",
              "jobChainPath": "/JOB-CHAIN",
              "nodeId": "100"
            }
          }
        ]
      }""")
  }

  private def check(q: JobDetailed, json: JsValue) = {
    assert(q.toJson == json)
    assert(json.convertTo[JobDetailed] == q)
  }
}
