package com.sos.scheduler.engine.kernel.order.jobchain

import com.google.inject.Injector
import com.sos.scheduler.engine.common.guice.GuiceImplicits._
import com.sos.scheduler.engine.common.scalautil.Logger
import com.sos.scheduler.engine.common.scalautil.ScalaUtils.implicits._
import com.sos.scheduler.engine.common.scalautil.xmls.ScalaStax._
import com.sos.scheduler.engine.cplusplus.runtime.annotation.ForCpp
import com.sos.scheduler.engine.cplusplus.runtime.{Sister, SisterType}
import com.sos.scheduler.engine.data.job.JobPath
import com.sos.scheduler.engine.data.jobchain.JobNodeOverview
import com.sos.scheduler.engine.data.order.OrderStateTransition
import com.sos.scheduler.engine.kernel.cppproxy.Job_nodeC
import com.sos.scheduler.engine.kernel.job.Job
import com.sos.scheduler.engine.kernel.order.Order
import com.sos.scheduler.engine.kernel.order.jobchain.JobChainNodeParserAndHandler.OrderFunction
import com.sos.scheduler.engine.kernel.order.jobchain.JobNode._
import com.sos.scheduler.engine.kernel.plugin.PluginSubsystem
import com.sos.scheduler.engine.kernel.plugin.jobchainnode.JobChainNodeNamespaceXmlPlugin
import com.sos.scheduler.engine.kernel.scheduler.HasInjector
import javax.xml.stream.XMLEventReader
import org.scalactic.Requirements._
import org.w3c.dom

@ForCpp
final class JobNode(
  protected val cppProxy: Job_nodeC,
  protected val injector: Injector)
extends OrderQueueNode with JobChainNodeParserAndHandler {

  @ForCpp
  def onOrderStepEnded(order: Order, returnCode: Int): Unit = {
    logger.trace(s"$this onOrderStepEnded ${order.id} returnCode=$returnCode")
    require(order.jobChainPath == jobChainPath)
    for (orderFunction ← returnCodeToOrderFunctions(returnCode)) {
      logger.trace(s"returnCode=$returnCode => calling plugin callback $orderFunction")
      orderFunction(order)
    }
  }

  override def processConfigurationDomElement(nodeElement: dom.Element) = {
    val pluginSubsystem = injector.apply[PluginSubsystem]
    initializeWithNodeXml(
      domElementToStaxSource(nodeElement),
      namespaceToOnReturnCodeParser =
        (for (plugin ← pluginSubsystem.plugins[JobChainNodeNamespaceXmlPlugin]) yield {
          plugin.xmlNamespace → { r: XMLEventReader ⇒ plugin.parseOnReturnCodeXml(this, r) withToString s"$plugin.onResultCode": OrderFunction }
        }).toMap)
    super.processConfigurationDomElement(nodeElement)
  }

  @ForCpp
  def orderStateTransitionToState(cppInternalValue: Long): String =
    orderStateTransitionToState(OrderStateTransition.ofCppInternalValue(cppInternalValue)).string

  override def toString = s"${getClass.getSimpleName} $nodeKey $jobPath"

  override def overview = JobNodeOverview(
    orderState = orderState,
    nextState = nextState,
    errorState = errorState,
    orderCount = orderCount,
    action = action,
    jobPath = jobPath)

  def jobPath: JobPath = JobPath(cppProxy.job_path)

  def getJob: Job = cppProxy.job.getSister
}

object JobNode {
  private val logger = Logger(getClass)

  final class Type extends SisterType[JobNode, Job_nodeC] {
    def sister(proxy: Job_nodeC, context: Sister): JobNode = {
      val injector = context.asInstanceOf[HasInjector].injector
      new JobNode(proxy, injector)
    }
  }
}
