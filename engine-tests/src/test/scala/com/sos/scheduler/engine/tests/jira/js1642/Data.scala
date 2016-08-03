package com.sos.scheduler.engine.tests.jira.js1642

import com.sos.scheduler.engine.common.sprayutils.JsObjectMarshallers._
import com.sos.scheduler.engine.common.utils.IntelliJUtils._
import com.sos.scheduler.engine.data.compounds.{OrderTreeComplemented, OrdersComplemented}
import com.sos.scheduler.engine.data.filebased.FileBasedState
import com.sos.scheduler.engine.data.folder.{FolderPath, FolderTree}
import com.sos.scheduler.engine.data.job.{JobOverview, JobPath, JobState, ProcessClassOverview, TaskId, TaskOverview, TaskState}
import com.sos.scheduler.engine.data.jobchain.{JobChainPath, NodeKey, SimpleJobNodeOverview}
import com.sos.scheduler.engine.data.order.{OrderObstacle, OrderOverview, OrderProcessingState, OrderSourceType, OrderState}
import com.sos.scheduler.engine.data.processclass.ProcessClassPath
import com.sos.scheduler.engine.tests.jira.js1642.Data._
import java.time.Instant
import java.time.Instant._
import spray.json.{JsArray, JsObject, _}

/**
  * @author Joacim Zschimmer
  */
private[js1642] final class Data(taskIdToStartedAt: TaskId ⇒ Instant) {
  val a1OrderOverview = OrderOverview(
    a1OrderKey,
    FileBasedState.active,
    OrderSourceType.fileBased,
    OrderState("100"),
    OrderProcessingState.InTaskProcess(TaskId(3), ProcessClassPath.Default, agentUri = None, taskIdToStartedAt(TaskId(3))),
    nextStepAt = Some(EPOCH))
  private val a1OrderOverviewJson = s"""{
    "path": "/aJobChain,1",
    "fileBasedState": "active",
    "sourceType": "fileBased",
    "orderState": "100",
    "processingState" : {
      "TYPE": "InTaskProcess",
      "taskId": "3",
      "processClassPath": "",
      "since": "${taskIdToStartedAt(TaskId(3))}"
    },
    "obstacles": [],
    "nextStepAt": "1970-01-01T00:00:00Z"
  }"""

  val a2OrderOverview = OrderOverview(
    a2OrderKey,
    FileBasedState.active,
    OrderSourceType.fileBased,
    OrderState("100"),
    OrderProcessingState.InTaskProcess(TaskId(4), ProcessClassPath.Default, agentUri = None, taskIdToStartedAt(TaskId(4))),
    nextStepAt = Some(EPOCH))
  private val a2OrderOverviewJson = s"""{
    "path": "/aJobChain,2",
    "fileBasedState": "active",
    "sourceType": "fileBased",
    "orderState": "100",
    "processingState" : {
      "TYPE": "InTaskProcess",
      "taskId": "4",
      "processClassPath": "",
      "since": "${taskIdToStartedAt(TaskId(4))}"
    },
    "obstacles": [],
    "nextStepAt": "1970-01-01T00:00:00Z"
  }"""

  val aAdHocOrderOverview = OrderOverview(
    aAdHocOrderKey,
    FileBasedState.not_initialized,
    OrderSourceType.adHoc,
    OrderState("100"),
    OrderProcessingState.Planned(OrderStartAt),
    Set(OrderObstacle.Suspended),
    nextStepAt = Some(OrderStartAt))
  private val aAdHocOrderOverviewJson = """{
    "path": "/aJobChain,AD-HOC",
    "fileBasedState": "not_initialized",
    "orderState": "100",
    "sourceType": "adHoc",
    "processingState" : {
      "TYPE": "Planned",
      "at": "2038-01-01T11:22:33Z"
    },
    "obstacles": [ "Suspended" ],
    "nextStepAt": "2038-01-01T11:22:33Z"
  }"""

  val b1OrderOverview = OrderOverview(
    b1OrderKey,
    FileBasedState.active,
    OrderSourceType.fileBased,
    OrderState("100"),
    OrderProcessingState.InTaskProcess(TaskId(5), ProcessClassPath.Default, agentUri = None, taskIdToStartedAt(TaskId(5))),
    nextStepAt = Some(EPOCH))
  private val b1OrderOverviewJson = s"""{
    "path": "/bJobChain,1",
    "fileBasedState": "active",
    "orderState": "100",
    "sourceType": "fileBased",
    "processingState" : {
      "TYPE": "InTaskProcess",
      "taskId": "5",
      "processClassPath": "",
      "since": "${taskIdToStartedAt(TaskId(5))}"
    },
    "obstacles": [],
    "nextStepAt": "1970-01-01T00:00:00Z"
  }"""

  val xa1OrderOverview = OrderOverview(
    xa1OrderKey,
    FileBasedState.active,
    OrderSourceType.fileBased,
    OrderState("100"),
    OrderProcessingState.Pending(EPOCH),
    nextStepAt = Some(EPOCH))
  private val xa1OrderOverviewJson = """{
    "path": "/xFolder/x-aJobChain,1",
    "fileBasedState": "active",
    "sourceType": "fileBased",
    "orderState": "100",
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
    OrderSourceType.fileBased,
    OrderState("100"),
    OrderProcessingState.Pending(EPOCH),
    Set(OrderObstacle.Suspended),
    nextStepAt = Some(EPOCH))
  private val xa2OrderOverviewJson = """{
    "path": "/xFolder/x-aJobChain,2",
    "fileBasedState": "active",
    "sourceType": "fileBased",
    "orderState": "100",
    "processingState" : {
      "TYPE": "Pending",
      "at" : "1970-01-01T00:00:00Z"
    },
    "obstacles": [ "Suspended" ],
    "nextStepAt": "1970-01-01T00:00:00Z"
  }"""

  val xb1OrderOverview = OrderOverview(
    xb1OrderKey,
    FileBasedState.not_initialized,
    OrderSourceType.fileBased,
    OrderState("100"),
    OrderProcessingState.Pending(EPOCH),
    nextStepAt = Some(EPOCH))
  private val xb1OrderOverviewJson = """{
    "path": "/xFolder/x-bJobChain,1",
    "fileBasedState": "not_initialized",
    "sourceType": "fileBased",
    "orderState": "100",
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
    OrderSourceType.adHoc,
    OrderState("100"),
    OrderProcessingState.Pending(EPOCH),
    nextStepAt = Some(EPOCH))
  private val xbAdHocDistributedOrderOverviewJson = """{
    "path": "/xFolder/x-bJobChain,AD-HOC-DISTRIBUTED",
    "fileBasedState": "not_initialized",
    "sourceType": "adHoc",
    "orderState": "100",
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

  val ExpectedOrderOrdersComplemented = OrdersComplemented(
    ExpectedOrderOverviews,
    Vector(
      SimpleJobNodeOverview(NodeKey(aJobChainPath, OrderState("100")), OrderState("END"), OrderState(""), TestJobPath, orderCount = 3),
      SimpleJobNodeOverview(NodeKey(bJobChainPath, OrderState("100")), OrderState("END"), OrderState(""), TestJobPath, orderCount = 1),
      SimpleJobNodeOverview(NodeKey(xaJobChainPath, OrderState("100")), OrderState("END"), OrderState(""), XFolderTestJobPath, orderCount = 2),
      SimpleJobNodeOverview(NodeKey(xbJobChainPath, OrderState("100")), OrderState("END"), OrderState(""), XFolderTestJobPath, orderCount = 0)),  // Distributed orders yet not counted
    Vector(
      JobOverview(TestJobPath, FileBasedState.active, defaultProcessClass = None, JobState.running, isInPeriod = true,
        taskLimit = 10, usedTaskCount = 3, obstacles = Set())),
    Vector(
      TaskOverview(TaskId(3), TestJobPath, TaskState.running, ProcessClassPath.Default),
      TaskOverview(TaskId(4), TestJobPath, TaskState.running, ProcessClassPath.Default),
      TaskOverview(TaskId(5), TestJobPath, TaskState.running, ProcessClassPath.Default)),
    Vector(
      ProcessClassOverview(ProcessClassPath.Default, FileBasedState.active, processLimit = 30, usedProcessCount = 3))
  )

  val ExpectedOrderTreeComplemented = OrderTreeComplemented(
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
    ExpectedOrderOrdersComplemented.usedNodes,
    ExpectedOrderOrdersComplemented.usedJobs,
    ExpectedOrderOrdersComplemented.usedTasks,
    ExpectedOrderOrdersComplemented.usedProcessClasses)

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

  val UsedTasksJson = """[
    {
      "taskId": "3",
      "jobPath": "/test",
      "state": "running",
      "processClass": ""
    },
    {
      "taskId": "4",
      "jobPath": "/test",
      "state": "running",
      "processClass": ""
    },
    {
      "taskId": "5",
      "jobPath": "/test",
      "state": "running",
      "processClass": ""
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
  val UsedProcessClassesJson = """[
    {
      "path": "",
      "fileBasedState": "active",
      "processLimit": 30,
      "usedProcessCount": 3
    }
  ]"""
  val UsedNodesJson = """[
    {
      "$TYPE": "SimpleJob",
      "nodeKey": {
        "jobChainPath": "/aJobChain",
        "state": "100"
      },
      "nextState": "END",
      "errorState": "",
      "jobPath": "/test",
      "action": "process",
      "orderCount": 3
    },
    {
      "$TYPE": "SimpleJob",
      "nodeKey": {
        "jobChainPath": "/bJobChain",
        "state": "100"
      },
      "nextState": "END",
      "errorState": "",
      "jobPath": "/test",
      "action": "process",
      "orderCount": 1
    },{
      "$TYPE": "SimpleJob",
      "nodeKey": {
        "jobChainPath": "/xFolder/x-aJobChain",
        "state": "100"
      },
      "nextState": "END",
      "errorState": "",
      "jobPath": "/xFolder/test",
      "action": "process",
      "orderCount": 2
    },
    {
      "$TYPE": "SimpleJob",
      "nodeKey": {
        "jobChainPath": "/xFolder/x-bJobChain",
        "state": "100"
      },
      "nextState": "END",
      "errorState": "",
      "jobPath": "/xFolder/test",
      "action": "process",
      "orderCount": 0
    }
  ]"""

  val ExpectedOrdersOrdersComplementedJsObject: JsObject = s"""{
    "orders": $ExpectedOrderOverviewsJsArray,
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
    "usedTasks": $UsedTasksJson,
    "usedJobs": $UsedJobsJson,
    "usedProcessClasses": $UsedProcessClassesJson,
    "usedNodes": $UsedNodesJson
  }""".parseJson.asJsObject
}

private[js1642] object Data {
  intelliJuseImports(JsObjectMarshaller)

  val aJobChainPath = JobChainPath("/aJobChain")

  val a1OrderKey = aJobChainPath orderKey "1"
  val a2OrderKey = aJobChainPath orderKey "2"
  val aAdHocOrderKey = aJobChainPath orderKey "AD-HOC"
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
