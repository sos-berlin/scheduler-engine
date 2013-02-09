package com.sos.scheduler.engine.playground.plugins.jobnet

import com.sos.scheduler.engine.data.folder.JobPath
import com.sos.scheduler.engine.data.order.{OrderState, OrderStepEndedEvent}
import com.sos.scheduler.engine.eventbus.{EventHandlerAnnotated, EventBus, HotEventHandler}
import com.sos.scheduler.engine.playground.plugins.jobnet.end.EndNode
import com.sos.scheduler.engine.playground.plugins.jobnet.node.JobNode
import com.sos.scheduler.engine.playground.plugins.jobnet.parallel.{JoinEntrance, BranchingExit, BranchId}
import com.sos.scheduler.engine.playground.plugins.jobnet.scheduler.{Order, JobChain, FileBasedCompanion}
import com.sos.scheduler.engine.playground.plugins.jobnet.serial.{SerialExit, SerialEntrance}

final class JobChainCompanion(jobChain: JobChain, jobNet: RawJobNet, eventBus: EventBus)
extends FileBasedCompanion
with EventHandlerAnnotated {

  def start() {
    eventBus.registerAnnotated(this)
  }

  def close() {
    eventBus.unregisterAnnotated(this)
  }

  @HotEventHandler def handleEvent(e: OrderStepEndedEvent, order: Order) {
    if (order.jobChain == jobChain) {
      jobNet.onOrderStepEnded(order)
    }
  }
}

object JobChainCompanion {
  def newCompanion(jobChain: JobChain, xmlString: String, eventBus: EventBus): (JobChainCompanion, String) = {
    val jobNet = convertXmlConfigurationToJobNet(jobChain, xmlString)
    (new JobChainCompanion(jobChain, jobNet, eventBus), jobNet.toSchedulerJobChainXml)
  }

  def convertXmlConfigurationToJobNet(jobChain: JobChain, xmlString: String): RawJobNet = TEST.myJobNet(jobChain)

  private object TEST {
    val startState = OrderState("start")
    val aState = OrderState("A")
    val xbState = OrderState("x/B")
    val xcState = OrderState("x/C")
    val ybState = OrderState("y/B")
    val ycState = OrderState("y/C")
    val dState = OrderState("D")

    val xBranchId = BranchId("a")
    val yBranchId = BranchId("b")

    val someJobPath = JobPath.of("/a")

    def myJobNet(jobChain: JobChain) = RawJobNet(jobChain, startState, Set(
      JobNode(SerialEntrance(aState), someJobPath, BranchingExit(Set(xBranchId -> xbState, yBranchId -> ybState))),
      JobNode(SerialEntrance(xbState), someJobPath, SerialExit(xcState)),
      JobNode(SerialEntrance(xcState), someJobPath, SerialExit(dState)),
      JobNode(SerialEntrance(ybState), someJobPath, SerialExit(ycState)),
      JobNode(SerialEntrance(ycState), someJobPath, SerialExit(dState)),
      EndNode(JoinEntrance(dState, Set(xBranchId, yBranchId)))))
  }
}