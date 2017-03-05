package com.sos.scheduler.engine.tests.jira.js1642

import com.sos.jobscheduler.common.sprayutils.JsObjectMarshallers._
import com.sos.jobscheduler.common.time.ScalaTime._
import com.sos.jobscheduler.common.utils.IntelliJUtils._
import com.sos.jobscheduler.data.folder.FolderPath
import com.sos.jobscheduler.data.job.TaskId
import com.sos.scheduler.engine.data.compounds.{OrderTreeComplemented, OrdersComplemented}
import com.sos.scheduler.engine.data.filebased.{FileBasedObstacle, FileBasedState}
import com.sos.scheduler.engine.data.folder.FolderTree
import com.sos.scheduler.engine.data.job.{JobObstacle, JobOverview, JobPath, JobState, TaskOverview, TaskState}
import com.sos.scheduler.engine.data.jobchain.{JobChainObstacle, JobChainOverview, JobChainPath, NodeId, NodeObstacle, SimpleJobNodeOverview}
import com.sos.scheduler.engine.data.lock.LockPath
import com.sos.scheduler.engine.data.monitor.MonitorPath
import com.sos.scheduler.engine.data.order.{OrderHistoryId, OrderId, OrderObstacle, OrderOverview, OrderProcessingState, OrderSourceType}
import com.sos.scheduler.engine.data.processclass.{ProcessClassDetailed, ProcessClassOverview, ProcessClassPath}
import com.sos.scheduler.engine.data.schedule.SchedulePath
import com.sos.scheduler.engine.tests.jira.js1642.Data._
import java.time.Instant
import java.time.Instant._
import spray.json._

/**
  * @author Joacim Zschimmer
  */
private[js1642] final class Data(taskIdToStartedAt: TaskId ⇒ Instant) {

  val DefaultProcessClassOverview = ProcessClassOverview(
    ProcessClassPath.Default,
    FileBasedState.active,
    processLimit = 30,
    usedProcessCount = 0)

  val TestProcessClassPath = ProcessClassPath("/test")
  val TestProcessClassOverview = ProcessClassOverview(
    TestProcessClassPath,
    FileBasedState.active,
    processLimit = 30,
    usedProcessCount = 3)

  val TestProcessClassDetailed = ProcessClassDetailed(
    TestProcessClassOverview,
    selectionMethod = "FixedPriority",
    processes = Nil,
    agents = Nil)

  val aJobChainOverview = JobChainOverview(aJobChainPath, FileBasedState.active)
  val bJobChainOverview = JobChainOverview(bJobChainPath, FileBasedState.active)
  val xaJobChainOverview = JobChainOverview(xaJobChainPath, FileBasedState.active,
    obstacles = Set(
      JobChainObstacle.FileBasedObstacles(Set(
        FileBasedObstacle.MissingRequisites(Set(
          XTestJobPath))))))
  val xbJobChainOverview = JobChainOverview(xbJobChainPath, FileBasedState.active, isDistributed = true,
    obstacles = Set(
      JobChainObstacle.FileBasedObstacles(Set(
        FileBasedObstacle.MissingRequisites(Set(
          LockPath("/xFolder/MISSING-LOCK"),
          MonitorPath("/xFolder/MISSING-MONITOR"),
          ProcessClassPath("/xFolder/MISSING-PROCESS-CLASS"),
          SchedulePath("/xFolder/MISSING-SCHEDULE"),
          XTestBJobPath))))))
  val nestedOuterJobChainOverview = JobChainOverview(nestedOuterJobChainPath, FileBasedState.active,
    orderIdSpaceName = Some("strawberries"))
  val nestedInnerJobChainOverview = JobChainOverview(nestedInnerJobChainPath, FileBasedState.active,
    orderIdSpaceName = Some("strawberries"))

  val a1OrderOverview = OrderOverview(
    a1OrderKey,
    FileBasedState.active,
    OrderSourceType.Permanent,
    a2OrderKey.jobChainPath,
    NodeId("100"),
    OrderProcessingState.InTaskProcess(TaskId(3), TestProcessClassPath, since = taskIdToStartedAt(TaskId(3)), agentUri = None),
    historyId = Some(OrderHistoryId(2)),
    nextStepAt = Some(EPOCH))
  private val a1OrderOverviewJson = s"""{
    "path": "/aJobChain,1",
    "fileBasedState": "active",
    "orderSourceType": "Permanent",
    "jobChainPath": "/aJobChain",
    "nodeId": "100",
    "orderProcessingState" : {
      "TYPE": "InTaskProcess",
      "taskId": "3",
      "processClassPath": "/test",
      "since": "${taskIdToStartedAt(TaskId(3))}"
    },
    "historyId": 2,
    "obstacles": [],
    "nextStepAt": "1970-01-01T00:00:00Z"
  }"""

  val a2OrderOverview = OrderOverview(
    a2OrderKey,
    FileBasedState.active,
    OrderSourceType.Permanent,
    a2OrderKey.jobChainPath,
    NodeId("100"),
    OrderProcessingState.InTaskProcess(TaskId(4), TestProcessClassPath, since = taskIdToStartedAt(TaskId(4)), agentUri = None),
    historyId = Some(OrderHistoryId(3)),
    nextStepAt = Some(EPOCH))
  private val a2OrderOverviewJson = s"""{
    "path": "/aJobChain,2",
    "fileBasedState": "active",
    "orderSourceType": "Permanent",
    "jobChainPath": "/aJobChain",
    "nodeId": "100",
    "orderProcessingState" : {
      "TYPE": "InTaskProcess",
      "taskId": "4",
      "processClassPath": "/test",
      "since": "${taskIdToStartedAt(TaskId(4))}"
    },
    "historyId": 3,
    "obstacles": [],
    "nextStepAt": "1970-01-01T00:00:00Z"
  }"""

  val aAdHocOrderOverview = OrderOverview(
    aAdHocOrderKey,
    FileBasedState.not_initialized,
    OrderSourceType.AdHoc,
    aAdHocOrderKey.jobChainPath,
    NodeId("100"),
    OrderProcessingState.Planned(OrderStartAt),
    obstacles = Set(OrderObstacle.Suspended),
    nextStepAt = Some(OrderStartAt))
  private val aAdHocOrderOverviewJson = """{
    "path": "/aJobChain,ÅD-HÖC",
    "fileBasedState": "not_initialized",
    "jobChainPath": "/aJobChain",
    "nodeId": "100",
    "orderSourceType": "AdHoc",
    "orderProcessingState" : {
      "TYPE": "Planned",
      "at": "2038-01-01T11:22:33Z"
    },
    "obstacles": [
      {
        "TYPE": "Suspended"
      }
    ],
    "nextStepAt": "2038-01-01T11:22:33Z"
  }"""

  val b1OrderOverview = OrderOverview(
    b1OrderKey,
    FileBasedState.active,
    OrderSourceType.Permanent,
    b1OrderKey.jobChainPath,
    NodeId("100"),
    OrderProcessingState.InTaskProcess(TaskId(5), TestProcessClassPath, since = taskIdToStartedAt(TaskId(5)), agentUri = None),
    historyId = Some(OrderHistoryId(4)),
    nextStepAt = Some(EPOCH),
    obstacles = Set(OrderObstacle.FileBasedObstacles(Set(FileBasedObstacle.Replaced()))))
  private val b1OrderOverviewJson = s"""{
    "path": "/bJobChain,1",
    "fileBasedState": "active",
    "jobChainPath": "/bJobChain",
    "nodeId": "100",
    "orderSourceType": "Permanent",
    "orderProcessingState" : {
      "TYPE": "InTaskProcess",
      "taskId": "5",
      "processClassPath": "/test",
      "since": "${taskIdToStartedAt(TaskId(5))}"
    },
    "historyId": 4,
    "nextStepAt": "1970-01-01T00:00:00Z",
    "obstacles": [
      {
        "TYPE": "FileBasedObstacles",
        "fileBasedObstacles": [
          {
            "TYPE": "Replaced"
          }
        ]
      }
    ]
  }"""

  val nestedOrderOverview = OrderOverview(
    nestedOrderKey,
    FileBasedState.active,
    OrderSourceType.Permanent,
    nestedInnerJobChainPath,
    NodeId("INNER-100"),
    OrderProcessingState.Due(EPOCH),
    obstacles = Set(OrderObstacle.Suspended),
    nextStepAt = Some(EPOCH),
    outerJobChainPath = Some(nestedOuterJobChainPath))
  private val nestedOrderOverviewJson = s"""{
    "path": "/outer/nested-outer,1",
    "fileBasedState": "active",
    "jobChainPath": "/nested-inner",
    "nodeId": "INNER-100",
    "outerJobChainPath": "/outer/nested-outer",
    "orderSourceType": "Permanent",
    "nextStepAt": "1970-01-01T00:00:00Z",
    "fileBasedState": "active",
    "orderProcessingState": {
      "TYPE": "Due",
      "at": "1970-01-01T00:00:00Z"
    },
    "obstacles": [
      {
        "TYPE": "Suspended"
      }
    ]
  }"""

  val xa1OrderOverview = OrderOverview(
    xa1OrderKey,
    FileBasedState.active,
    OrderSourceType.Permanent,
    xa1OrderKey.jobChainPath,
    NodeId("100"),
    OrderProcessingState.Due(EPOCH),
    nextStepAt = Some(EPOCH),
    obstacles = Set(
      OrderObstacle.FileBasedObstacles(Set(
        FileBasedObstacle.MissingRequisites(Set(
          XTestJobPath))))))
  private val xa1OrderOverviewJson = """{
    "path": "/xFolder/x-aJobChain,1",
    "fileBasedState": "active",
    "orderSourceType": "Permanent",
    "jobChainPath": "/xFolder/x-aJobChain",
    "nodeId": "100",
    "orderProcessingState" : {
      "TYPE": "Due",
      "at" : "1970-01-01T00:00:00Z"
    },
    "obstacles": [
      {
        "TYPE": "FileBasedObstacles",
        "fileBasedObstacles": [
          {
            "TYPE": "MissingRequisites",
            "paths": [
              "Job:/xFolder/test"
            ]
          }
        ]
      }
    ],
    "nextStepAt": "1970-01-01T00:00:00Z"
  }"""

  val xa2OrderOverview = OrderOverview(
    xa2OrderKey,
    FileBasedState.active,
    OrderSourceType.Permanent,
    xa2OrderKey.jobChainPath,
    NodeId("100"),
    OrderProcessingState.Due(EPOCH),
    nextStepAt = Some(EPOCH),
    obstacles = Set(
      OrderObstacle.Suspended,
      OrderObstacle.FileBasedObstacles(Set(
        FileBasedObstacle.MissingRequisites(Set(
          XTestJobPath))))))
  private val xa2OrderOverviewJson = """{
    "path": "/xFolder/x-aJobChain,2",
    "fileBasedState": "active",
    "orderSourceType": "Permanent",
    "jobChainPath": "/xFolder/x-aJobChain",
    "nodeId": "100",
    "orderProcessingState" : {
      "TYPE": "Due",
      "at" : "1970-01-01T00:00:00Z"
    },
    "obstacles": [
      {
        "TYPE": "FileBasedObstacles",
        "fileBasedObstacles": [
          {
            "TYPE": "MissingRequisites",
            "paths": [
              "Job:/xFolder/test"
            ]
          }
        ]
      },
      {
        "TYPE": "Suspended"
      }
    ],
    "nextStepAt": "1970-01-01T00:00:00Z"
  }"""

  val xb1OrderOverview = OrderOverview(
    xb1OrderKey,
    FileBasedState.not_initialized,
    OrderSourceType.Permanent,
    xb1OrderKey.jobChainPath,
    NodeId("100"),
    OrderProcessingState.Due(EPOCH),
    nextStepAt = Some(EPOCH))
  private val xb1OrderOverviewJson = """{
    "path": "/xFolder/x-bJobChain,1",
    "fileBasedState": "not_initialized",
    "orderSourceType": "Permanent",
    "jobChainPath": "/xFolder/x-bJobChain",
    "nodeId": "100",
    "orderProcessingState" : {
      "TYPE": "Due",
      "at" : "1970-01-01T00:00:00Z"
    },
    "obstacles": [],
    "nextStepAt": "1970-01-01T00:00:00Z"
  }"""

  val xbAdHocDistributedOrderOverview = OrderOverview(
    xbAdHocDistributedOrderKey,
    FileBasedState.not_initialized,
    OrderSourceType.AdHoc,
    xbAdHocDistributedOrderKey.jobChainPath,
    NodeId("100"),
    OrderProcessingState.Due(EPOCH),
    nextStepAt = Some(EPOCH))
  private val xbAdHocDistributedOrderOverviewJson = """{
    "path": "/xFolder/x-bJobChain,AD-HOC-DISTRIBUTED",
    "fileBasedState": "not_initialized",
    "orderSourceType": "AdHoc",
    "jobChainPath": "/xFolder/x-bJobChain",
    "nodeId": "100",
    "orderProcessingState" : {
      "TYPE": "Due",
      "at" : "1970-01-01T00:00:00Z"
    },
    "obstacles": [],
    "nextStepAt": "1970-01-01T00:00:00Z"
  }"""

  val ExpectedOrderOverviews = Vector(
    a1OrderOverview,
    a2OrderOverview,
    aAdHocOrderOverview,
    b1OrderOverview,
    xa1OrderOverview,
    xa2OrderOverview,
    nestedOrderOverview,
    xb1OrderOverview,
    xbAdHocDistributedOrderOverview)

  val A100NodeOverview = SimpleJobNodeOverview(aJobChainPath, NodeId("100"), NodeId("END"), NodeId(""),
    TestJobPath, orderCount = 3, obstacles = Set(NodeObstacle.Delaying(999999.s)))
  val B100NodeOverview = SimpleJobNodeOverview(bJobChainPath, NodeId("100"), NodeId("END"), NodeId(""),
    TestJobPath, orderCount = 1)
  //val NestedOuter1NodeOverview = SimpleJobNodeOverview(bJobChainPath, NodeId("OUTER-1"), NodeId("END"), NodeId(""),
  //  TestJobPath, orderCount = 1)
  val NestedInner100NodeOverview = SimpleJobNodeOverview(nestedInnerJobChainPath, NodeId("INNER-100"), NodeId("END"), NodeId(""),
    TestJobPath, orderCount = 1)
  val Xa100NodeOverview = SimpleJobNodeOverview(xaJobChainPath, NodeId("100"), NodeId("END"), NodeId(""),
    XTestJobPath, orderCount = 2,
    obstacles = Set(NodeObstacle.MissingJob(XTestJobPath)))
  val Xb100NodeOverview = SimpleJobNodeOverview(xbJobChainPath, NodeId("100"), NodeId("END"), NodeId(""),
    XTestBJobPath, orderCount = 0, obstacles = Set(NodeObstacle.WaitingForJob))

  val TestJobOverview = JobOverview(TestJobPath, FileBasedState.active, defaultProcessClassPath = Some(TestProcessClassPath),
    JobState.running, isInPeriod = true, taskLimit = 10, usedTaskCount = 3, obstacles = Set())
  val XTestBJobOverview = JobOverview(XTestBJobPath, FileBasedState.incomplete, defaultProcessClassPath = Some(ProcessClassPath("/xFolder/MISSING-PROCESS-CLASS")),
    JobState.loaded, isInPeriod = false,
    taskLimit = 1, usedTaskCount = 0,
    obstacles = Set(
      JobObstacle.FileBasedObstacles(Set(
        FileBasedObstacle.BadState(FileBasedState.incomplete),
        FileBasedObstacle.MissingRequisites(Set(
          LockPath("/xFolder/MISSING-LOCK"),
          MonitorPath("/xFolder/MISSING-MONITOR"),
          ProcessClassPath("/xFolder/MISSING-PROCESS-CLASS"),
          SchedulePath("/xFolder/MISSING-SCHEDULE")))))))

  val ExpectedOrdersComplemented = OrdersComplemented(
    ExpectedOrderOverviews,
    Vector(
      aJobChainOverview,
      bJobChainOverview,
      nestedInnerJobChainOverview,
      nestedOuterJobChainOverview,
      xaJobChainOverview,
      xbJobChainOverview),
    Vector(
      A100NodeOverview,
      B100NodeOverview,
      NestedInner100NodeOverview,
      Xa100NodeOverview,
      Xb100NodeOverview),
    Vector(
      TestJobOverview,
      XTestBJobOverview),
    Vector(
      TaskOverview(TaskId(3), TestJobPath, TaskState.running, Some(TestProcessClassPath)),
      TaskOverview(TaskId(4), TestJobPath, TaskState.running, Some(TestProcessClassPath)),
      TaskOverview(TaskId(5), TestJobPath, TaskState.running, Some(TestProcessClassPath))),
    Vector(
      TestProcessClassOverview)
  )

  val ExpectedSuspendedOrdersComplemented = ExpectedOrdersComplemented.copy(
    orders = ExpectedOrdersComplemented.orders filter { _.isSuspended },
    usedJobChains = Vector(aJobChainOverview, nestedInnerJobChainOverview, nestedOuterJobChainOverview, xaJobChainOverview),
    usedTasks = Nil,
    usedJobs = Vector(
      TestJobOverview),
    usedProcessClasses = Vector(
      TestProcessClassOverview),
    usedNodes = Vector(
      A100NodeOverview,
      NestedInner100NodeOverview,
      Xa100NodeOverview))

  val ExpectedOrderTreeComplemented = OrderTreeComplemented[OrderOverview](
    FolderTree(
      FolderPath.Root,
      Vector(
        a1OrderOverview,
        a2OrderOverview,
        aAdHocOrderOverview,
        b1OrderOverview),
      Vector(
        FolderTree(
          FolderPath("/outer"),
          Vector(
            nestedOrderOverview),
          Vector()),
        FolderTree(
          FolderPath("/xFolder"),
          Vector(
            xa1OrderOverview,
            xa2OrderOverview,
            xb1OrderOverview,
            xbAdHocDistributedOrderOverview),
          Vector()))),
    ExpectedOrdersComplemented.usedNodes,
    ExpectedOrdersComplemented.usedJobs,
    ExpectedOrdersComplemented.usedTasks,
    ExpectedOrdersComplemented.usedProcessClasses)

  val OrderCount = ExpectedOrderOverviews.size

  val ExpectedOrderOverviewsJsArray: JsArray = s"""[
    $a1OrderOverviewJson,
    $a2OrderOverviewJson,
    $aAdHocOrderOverviewJson,
    $b1OrderOverviewJson,
    $xa1OrderOverviewJson,
    $xa2OrderOverviewJson,
    $nestedOrderOverviewJson,
    $xb1OrderOverviewJson,
    $xbAdHocDistributedOrderOverviewJson
  ]""".parseJson.asInstanceOf[JsArray]

  val UsedJobChainsJson = """[
      {
        "path": "/aJobChain",
        "fileBasedState": "active",
        "isDistributed": false,
        "obstacles": []
      },
      {
        "path": "/bJobChain",
        "fileBasedState": "active",
        "isDistributed": false,
        "obstacles": []
      },
      {
        "path": "/nested-inner",
        "fileBasedState": "active",
        "isDistributed": false,
        "obstacles": [],
        "orderIdSpaceName": "strawberries"
      },
      {
        "path": "/outer/nested-outer",
        "fileBasedState": "active",
        "isDistributed": false,
        "obstacles": [],
        "orderIdSpaceName": "strawberries"
      },
      {
        "path": "/xFolder/x-aJobChain",
        "fileBasedState": "active",
        "isDistributed": false,
        "obstacles": [
          {
            "TYPE":"FileBasedObstacles",
            "fileBasedObstacles": [
              {
                "TYPE": "MissingRequisites",
                "paths": [
                  "Job:/xFolder/test"
                ]
              }
            ]
          }
        ]
      },
      {
        "path": "/xFolder/x-bJobChain",
        "fileBasedState": "active",
        "isDistributed": true,
        "obstacles": [
          {
            "TYPE": "FileBasedObstacles",
            "fileBasedObstacles": [
              {
                "TYPE":"MissingRequisites",
                "paths": [
                  "ProcessClass:/xFolder/MISSING-PROCESS-CLASS",
                  "Job:/xFolder/test-b",
                  "Schedule:/xFolder/MISSING-SCHEDULE",
                  "Lock:/xFolder/MISSING-LOCK",
                  "Monitor:/xFolder/MISSING-MONITOR"
                ]
              }
            ]
          }
        ]
      }
    ]"""
  val UsedNodesJson = """[
    {
      "TYPE": "Job",
      "jobChainPath": "/aJobChain",
      "nodeId": "100",
      "nextNodeId": "END",
      "errorNodeId": "",
      "jobPath": "/test",
      "action": "process",
      "orderCount": 3,
      "obstacles": [
        {
          "duration": 999999,
          "TYPE": "Delaying"
        }
      ]
    },
    {
      "TYPE": "Job",
      "jobChainPath": "/bJobChain",
      "nodeId": "100",
      "nextNodeId": "END",
      "errorNodeId": "",
      "jobPath": "/test",
      "action": "process",
      "orderCount": 1,
      "obstacles": []
    },
    {
      "TYPE": "Job",
      "errorNodeId": "",
      "jobChainPath": "/nested-inner",
      "nodeId": "INNER-100",
      "nextNodeId": "END",
      "jobPath": "/test",
      "obstacles": [],
      "action": "process",
      "orderCount": 1
    },
    {
      "TYPE": "Job",
      "jobChainPath": "/xFolder/x-aJobChain",
      "nodeId": "100",
      "nextNodeId": "END",
      "errorNodeId": "",
      "jobPath": "/xFolder/test",
      "action": "process",
      "orderCount": 2,
      "obstacles": [
        {
          "TYPE": "MissingJob",
          "jobPath": "/xFolder/test"
        }
      ]
    },
    {
      "TYPE": "Job",
      "jobChainPath": "/xFolder/x-bJobChain",
      "nodeId": "100",
      "nextNodeId": "END",
      "errorNodeId": "",
      "jobPath": "/xFolder/test-b",
      "action": "process",
      "orderCount": 0,
      "obstacles": [
        {
          "TYPE": "WaitingForJob"
        }
      ]
    }
  ]"""
  val UsedJobsJson = """[
    {
      "path": "/test",
      "fileBasedState": "active",
      "taskLimit": 10,
      "state": "running",
      "isInPeriod": true,
      "defaultProcessClassPath": "/test",
      "usedTaskCount": 3,
      "obstacles": []
    },
    {
      "path": "/xFolder/test-b",
      "fileBasedState": "incomplete",
      "taskLimit": 1,
      "state": "loaded",
      "isInPeriod": false,
      "usedTaskCount": 0,
      "defaultProcessClassPath": "/xFolder/MISSING-PROCESS-CLASS",
      "obstacles": [
        {
          "TYPE":"FileBasedObstacles",
          "fileBasedObstacles": [
            {
              "TYPE":"BadState",
              "state": "incomplete"
            },
            {
              "TYPE":"MissingRequisites",
              "paths": [
                "ProcessClass:/xFolder/MISSING-PROCESS-CLASS",
                "Schedule:/xFolder/MISSING-SCHEDULE",
                "Lock:/xFolder/MISSING-LOCK",
                "Monitor:/xFolder/MISSING-MONITOR"
              ]
            }
          ]
        }
      ]
    }
  ]"""
  val UsedTasksJson = """[
    {
      "taskId": "3",
      "jobPath": "/test",
      "state": "running",
      "processClassPath": "/test",
      "obstacles": []
    },
    {
      "taskId": "4",
      "jobPath": "/test",
      "state": "running",
      "processClassPath": "/test",
      "obstacles": []
    },
    {
      "taskId": "5",
      "jobPath": "/test",
      "state": "running",
      "processClassPath": "/test",
      "obstacles": []
    }
  ]"""
  val UsedProcessClassesJson = """[
    {
      "path": "/test",
      "fileBasedState": "active",
      "processLimit": 30,
      "usedProcessCount": 3,
      "obstacles": []
    }
  ]"""

  val ExpectedOrdersOrdersComplementedJsObject: JsObject = s"""{
    "orders": $ExpectedOrderOverviewsJsArray,
    "usedJobChains": $UsedJobChainsJson,
    "usedTasks": $UsedTasksJson,
    "usedJobs": $UsedJobsJson,
    "usedProcessClasses": $UsedProcessClassesJson,
    "usedNodes": $UsedNodesJson
  }""".parseJson.asJsObject

  val ExpectedOrderTreeComplementedJsObject: JsObject = s"""{
    "orderTree": {
      "path": "/",
      "leafs": [
        $a1OrderOverviewJson,
        $a2OrderOverviewJson,
        $aAdHocOrderOverviewJson,
        $b1OrderOverviewJson
      ],
      "subfolders": [
        {
          "path": "/outer",
          "leafs": [
            $nestedOrderOverviewJson
          ],
          "subfolders": []
        },
        {
          "path": "/xFolder",
          "leafs": [
            $xa1OrderOverviewJson,
            $xa2OrderOverviewJson,
            $xb1OrderOverviewJson,
            $xbAdHocDistributedOrderOverviewJson
          ],
          "subfolders": []
        }
      ]
    },
    "usedNodes": $UsedNodesJson,
    "usedJobs": $UsedJobsJson,
    "usedTasks": $UsedTasksJson,
    "usedProcessClasses": $UsedProcessClassesJson
  }""".parseJson.asJsObject
}

private[js1642] object Data {
  intelliJuseImports(JsObjectMarshaller)

  val aJobChainPath = JobChainPath("/aJobChain")

  val OneOrderId = OrderId("1")
  val a1OrderKey = aJobChainPath orderKey OneOrderId
  val a2OrderKey = aJobChainPath orderKey "2"
  val aAdHocOrderKey = aJobChainPath orderKey "ÅD-HÖC"
  val bJobChainPath = JobChainPath("/bJobChain")
  val b1OrderKey = bJobChainPath orderKey OneOrderId
  val xFolderPath = FolderPath("/xFolder")
  val xaJobChainPath = JobChainPath("/xFolder/x-aJobChain")
  val xa1OrderKey = xaJobChainPath orderKey OneOrderId
  val xa2OrderKey = xaJobChainPath orderKey "2"
  val xbJobChainPath = JobChainPath("/xFolder/x-bJobChain")
  val nestedOuterJobChainPath = JobChainPath("/outer/nested-outer")
  val nestedInnerJobChainPath = JobChainPath("/nested-inner")

  val xb1OrderKey = xbJobChainPath orderKey OneOrderId
  val xbAdHocDistributedOrderKey = xbJobChainPath orderKey "AD-HOC-DISTRIBUTED"
  val nestedOrderKey = nestedOuterJobChainPath orderKey "1"
  val AllOrderKeys = Set(a1OrderKey, a2OrderKey, b1OrderKey, xa1OrderKey, xa2OrderKey, nestedOrderKey)
  val ProcessableOrderKeys = Vector(a1OrderKey, a2OrderKey, b1OrderKey)

  private[js1642] val OrderStartAt = Instant.parse("2038-01-01T11:22:33Z")

  private[js1642] val TestJobPath = JobPath("/test")
  private[js1642] val XTestJobPath = JobPath("/xFolder/test")
  private[js1642] val XTestBJobPath = JobPath("/xFolder/test-b")
}
