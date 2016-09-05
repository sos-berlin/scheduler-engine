package com.sos.scheduler.engine.tests.jira.js1642

import com.sos.scheduler.engine.common.sprayutils.JsObjectMarshallers._
import com.sos.scheduler.engine.common.utils.IntelliJUtils._
import com.sos.scheduler.engine.data.compounds.{OrderTreeComplemented, OrdersComplemented}
import com.sos.scheduler.engine.data.filebased.{FileBasedObstacle, FileBasedState}
import com.sos.scheduler.engine.data.folder.{FolderPath, FolderTree}
import com.sos.scheduler.engine.data.job.{JobOverview, JobPath, JobState, ProcessClassOverview, TaskId, TaskOverview, TaskState}
import com.sos.scheduler.engine.data.jobchain.{JobChainOverview, JobChainPath, NodeId, NodeKey, SimpleJobNodeOverview}
import com.sos.scheduler.engine.data.order.{OrderHistoryId, OrderObstacle, OrderOverview, OrderProcessingState, OrderSourceType}
import com.sos.scheduler.engine.data.processclass.ProcessClassPath
import com.sos.scheduler.engine.tests.jira.js1642.Data._
import java.time.Instant
import java.time.Instant._
import spray.json.{JsArray, JsObject, _}

/**
  * @author Joacim Zschimmer
  */
private[js1642] final class Data(taskIdToStartedAt: TaskId ⇒ Instant) {
  val aJobChainOverview = JobChainOverview(aJobChainPath, FileBasedState.active)
  val bJobChainOverview = JobChainOverview(bJobChainPath, FileBasedState.active)
  val xaJobChainOverview = JobChainOverview(xaJobChainPath, FileBasedState.active)
  val xbJobChainOverview = JobChainOverview(xbJobChainPath, FileBasedState.active, isDistributed = true)

  val a1OrderOverview = OrderOverview(
    a1OrderKey,
    FileBasedState.active,
    OrderSourceType.Permanent,
    NodeId("100"),
    OrderProcessingState.InTaskProcess(TaskId(3), ProcessClassPath.Default, agentUri = None, taskIdToStartedAt(TaskId(3))),
    historyId = Some(OrderHistoryId(2)),
    nextStepAt = Some(EPOCH))
  private val a1OrderOverviewJson = s"""{
    "path": "/aJobChain,1",
    "fileBasedState": "active",
    "sourceType": "Permanent",
    "nodeId": "100",
    "processingState" : {
      "TYPE": "InTaskProcess",
      "taskId": "3",
      "processClassPath": "",
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
    NodeId("100"),
    OrderProcessingState.InTaskProcess(TaskId(4), ProcessClassPath.Default, agentUri = None, taskIdToStartedAt(TaskId(4))),
    historyId = Some(OrderHistoryId(3)),
    nextStepAt = Some(EPOCH))
  private val a2OrderOverviewJson = s"""{
    "path": "/aJobChain,2",
    "fileBasedState": "active",
    "sourceType": "Permanent",
    "nodeId": "100",
    "processingState" : {
      "TYPE": "InTaskProcess",
      "taskId": "4",
      "processClassPath": "",
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
    NodeId("100"),
    OrderProcessingState.Planned(OrderStartAt),
    obstacles = Set(OrderObstacle.Suspended),
    nextStepAt = Some(OrderStartAt))
  private val aAdHocOrderOverviewJson = """{
    "path": "/aJobChain,ÅD-HÖC",
    "fileBasedState": "not_initialized",
    "nodeId": "100",
    "sourceType": "AdHoc",
    "processingState" : {
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
    NodeId("100"),
    OrderProcessingState.InTaskProcess(TaskId(5), ProcessClassPath.Default, agentUri = None, taskIdToStartedAt(TaskId(5))),
    historyId = Some(OrderHistoryId(4)),
    nextStepAt = Some(EPOCH),
    obstacles = Set(OrderObstacle.FileBasedObstacles(Set(FileBasedObstacle.Replaced))))
  private val b1OrderOverviewJson = s"""{
    "path": "/bJobChain,1",
    "fileBasedState": "active",
    "nodeId": "100",
    "sourceType": "Permanent",
    "processingState" : {
      "TYPE": "InTaskProcess",
      "taskId": "5",
      "processClassPath": "",
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

  val xa1OrderOverview = OrderOverview(
    xa1OrderKey,
    FileBasedState.active,
    OrderSourceType.Permanent,
    NodeId("100"),
    OrderProcessingState.Pending(EPOCH),
    nextStepAt = Some(EPOCH))
  private val xa1OrderOverviewJson = """{
    "path": "/xFolder/x-aJobChain,1",
    "fileBasedState": "active",
    "sourceType": "Permanent",
    "nodeId": "100",
    "processingState" : {
      "TYPE": "Pending",
      "at" : "1970-01-01T00:00:00Z"
    },
    "obstacles": [],
    "nextStepAt": "1970-01-01T00:00:00Z"
  }"""

  val xa2OrderOverview = OrderOverview(
    xa2OrderKey,
    FileBasedState.active,
    OrderSourceType.Permanent,
    NodeId("100"),
    OrderProcessingState.Pending(EPOCH),
    obstacles = Set(OrderObstacle.Suspended),
    nextStepAt = Some(EPOCH))
  private val xa2OrderOverviewJson = """{
    "path": "/xFolder/x-aJobChain,2",
    "fileBasedState": "active",
    "sourceType": "Permanent",
    "nodeId": "100",
    "processingState" : {
      "TYPE": "Pending",
      "at" : "1970-01-01T00:00:00Z"
    },
    "obstacles": [
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
    NodeId("100"),
    OrderProcessingState.Pending(EPOCH),
    nextStepAt = Some(EPOCH))
  private val xb1OrderOverviewJson = """{
    "path": "/xFolder/x-bJobChain,1",
    "fileBasedState": "not_initialized",
    "sourceType": "Permanent",
    "nodeId": "100",
    "processingState" : {
      "TYPE": "Pending",
      "at" : "1970-01-01T00:00:00Z"
    },
    "obstacles": [],
    "nextStepAt": "1970-01-01T00:00:00Z"
  }"""

  val xbAdHocDistributedOrderOverview = OrderOverview(
    xbAdHocDistributedOrderKey,
    FileBasedState.not_initialized,
    OrderSourceType.AdHoc,
    NodeId("100"),
    OrderProcessingState.Pending(EPOCH),
    nextStepAt = Some(EPOCH))
  private val xbAdHocDistributedOrderOverviewJson = """{
    "path": "/xFolder/x-bJobChain,AD-HOC-DISTRIBUTED",
    "fileBasedState": "not_initialized",
    "sourceType": "AdHoc",
    "nodeId": "100",
    "processingState" : {
      "TYPE": "Pending",
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
    xb1OrderOverview,
    xbAdHocDistributedOrderOverview)

  val ExpectedOrdersComplemented = OrdersComplemented(
    ExpectedOrderOverviews,
    Vector(
      aJobChainOverview,
      bJobChainOverview,
      xaJobChainOverview,
      xbJobChainOverview),
    Vector(
      SimpleJobNodeOverview(NodeKey(aJobChainPath, NodeId("100")), NodeId("END"), NodeId(""), TestJobPath, orderCount = 3),
      SimpleJobNodeOverview(NodeKey(bJobChainPath, NodeId("100")), NodeId("END"), NodeId(""), TestJobPath, orderCount = 1),
      SimpleJobNodeOverview(NodeKey(xaJobChainPath, NodeId("100")), NodeId("END"), NodeId(""), XFolderTestJobPath, orderCount = 2),
      SimpleJobNodeOverview(NodeKey(xbJobChainPath, NodeId("100")), NodeId("END"), NodeId(""), XFolderTestJobPath, orderCount = 0)),  // Distributed orders yet not counted
    Vector(
      JobOverview(TestJobPath, FileBasedState.active, defaultProcessClassPath = None, JobState.running, isInPeriod = true,
        taskLimit = 10, usedTaskCount = 3, obstacles = Set())),
    Vector(
      TaskOverview(TaskId(3), TestJobPath, TaskState.running, ProcessClassPath.Default),
      TaskOverview(TaskId(4), TestJobPath, TaskState.running, ProcessClassPath.Default),
      TaskOverview(TaskId(5), TestJobPath, TaskState.running, ProcessClassPath.Default)),
    Vector(
      ProcessClassOverview(ProcessClassPath.Default, FileBasedState.active, processLimit = 30, usedProcessCount = 3))
  )

  val ExpectedSuspendedOrdersComplemented = ExpectedOrdersComplemented.copy(
    orders = ExpectedOrdersComplemented.orders filter { _.isSuspended },
    usedJobChains = Vector(aJobChainOverview, xaJobChainOverview),
    usedTasks = Nil,
    usedJobs = ExpectedOrdersComplemented.usedJobs,
    usedProcessClasses = Nil,
    usedNodes = Vector(
      SimpleJobNodeOverview(NodeKey(aJobChainPath, NodeId("100")), NodeId("END"), NodeId(""), TestJobPath, orderCount = 3),
      SimpleJobNodeOverview(NodeKey(xaJobChainPath, NodeId("100")), NodeId("END"), NodeId(""), XFolderTestJobPath, orderCount = 2)))

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
          "path": "/xFolder/x-aJobChain",
          "fileBasedState": "active",
          "isDistributed": false,
          "obstacles": []
      },
      {
          "path": "/xFolder/x-bJobChain",
          "fileBasedState": "active",
          "isDistributed": true,
          "obstacles": []
      }
    ]"""
  val UsedNodesJson = """[
    {
      "TYPE": "SimpleJob",
      "nodeKey": {
        "jobChainPath": "/aJobChain",
        "nodeId": "100"
      },
      "nextNodeId": "END",
      "errorNodeId": "",
      "jobPath": "/test",
      "action": "process",
      "orderCount": 3
    },
    {
      "TYPE": "SimpleJob",
      "nodeKey": {
        "jobChainPath": "/bJobChain",
        "nodeId": "100"
      },
      "nextNodeId": "END",
      "errorNodeId": "",
      "jobPath": "/test",
      "action": "process",
      "orderCount": 1
    },{
      "TYPE": "SimpleJob",
      "nodeKey": {
        "jobChainPath": "/xFolder/x-aJobChain",
        "nodeId": "100"
      },
      "nextNodeId": "END",
      "errorNodeId": "",
      "jobPath": "/xFolder/test",
      "action": "process",
      "orderCount": 2
    },
    {
      "TYPE": "SimpleJob",
      "nodeKey": {
        "jobChainPath": "/xFolder/x-bJobChain",
        "nodeId": "100"
      },
      "nextNodeId": "END",
      "errorNodeId": "",
      "jobPath": "/xFolder/test",
      "action": "process",
      "orderCount": 0
    }
  ]"""
  val UsedJobsJson = """[
    {
      "path": "/test",
      "fileBasedState": "active",
      "taskLimit": 10,
      "state": "running",
      "isInPeriod": true,
      "usedTaskCount": 3,
      "obstacles": []
    }
  ]"""
  val UsedTasksJson = """[
    {
      "taskId": "3",
      "jobPath": "/test",
      "state": "running",
      "processClassPath": ""
    },
    {
      "taskId": "4",
      "jobPath": "/test",
      "state": "running",
      "processClassPath": ""
    },
    {
      "taskId": "5",
      "jobPath": "/test",
      "state": "running",
      "processClassPath": ""
    }
  ]"""
  val UsedProcessClassesJson = """[
    {
      "path": "",
      "fileBasedState": "active",
      "processLimit": 30,
      "usedProcessCount": 3
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

  val a1OrderKey = aJobChainPath orderKey "1"
  val a2OrderKey = aJobChainPath orderKey "2"
  val aAdHocOrderKey = aJobChainPath orderKey "ÅD-HÖC"
  val bJobChainPath = JobChainPath("/bJobChain")
  val b1OrderKey = bJobChainPath orderKey "1"
  val xaJobChainPath = JobChainPath("/xFolder/x-aJobChain")
  val xa1OrderKey = xaJobChainPath orderKey "1"
  val xa2OrderKey = xaJobChainPath orderKey "2"
  val xbJobChainPath = JobChainPath("/xFolder/x-bJobChain")

  val xb1OrderKey = xbJobChainPath orderKey "1"
  val xbAdHocDistributedOrderKey = xbJobChainPath orderKey "AD-HOC-DISTRIBUTED"
  val ProcessableOrderKeys = Vector(a1OrderKey, a2OrderKey, b1OrderKey)

  private[js1642] val OrderStartAt = Instant.parse("2038-01-01T11:22:33Z")

  private[js1642] val TestJobPath = JobPath("/test")
  private[js1642] val XFolderTestJobPath = JobPath("/xFolder/test")
}
