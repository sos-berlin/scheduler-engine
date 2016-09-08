package com.sos.scheduler.engine.kernel.order.jobchain

import com.sos.scheduler.engine.base.utils.ScalaUtils.SwitchStatement
import com.sos.scheduler.engine.common.guice.GuiceImplicits._
import com.sos.scheduler.engine.common.scalautil.Logger
import com.sos.scheduler.engine.common.scalautil.xmls.ScalaStax._
import com.sos.scheduler.engine.cplusplus.runtime.annotation.ForCpp
import com.sos.scheduler.engine.data.job.{JobPath, ReturnCode}
import com.sos.scheduler.engine.data.jobchain.{JobChainNodeAction, JobNodeOverview, NodeObstacle}
import com.sos.scheduler.engine.data.order.OrderNodeTransition
import com.sos.scheduler.engine.kernel.job.JobSubsystem
import com.sos.scheduler.engine.kernel.order.Order
import com.sos.scheduler.engine.kernel.order.jobchain.JobNode.logger
import com.sos.scheduler.engine.kernel.plugin.PluginSubsystem
import com.sos.scheduler.engine.kernel.plugin.jobchainnode.JobChainNodeNamespaceXmlPlugin
import org.scalactic.Requirements._
import org.w3c.dom

/**
 * @author Joacim Zschimmer
 */
abstract class JobNode extends OrderQueueNode with JobChainNodeParserAndHandler {

  private[kernel] def overview: JobNodeOverview
  def jobPath: JobPath

  protected final val jobSubsystem = injector.instance[JobSubsystem]

  override private[kernel] def processConfigurationDomElement(nodeElement: dom.Element) = {
    val namespaceToJobNodePlugins = injector.instance[PluginSubsystem].xmlNamespaceToPlugins[JobChainNodeNamespaceXmlPlugin] _
    initializeWithNodeXml(
      domElementToStaxSource(nodeElement),
      namespaceToJobNodePlugins(_) map { plugin ⇒ plugin.parseOnReturnCodeXml(this, _) })
    super.processConfigurationDomElement(nodeElement)
  }

  @ForCpp
  private final def onOrderStepEnded(order: Order, returnCode: Int): Unit = {
    logger.trace(s"$this onOrderStepEnded ${order.id} returnCode=$returnCode")
    require(order.jobChainPath == jobChainPath)
    for (orderFunction ← returnCodeToOrderFunctions(ReturnCode(returnCode))) {
      logger.trace(s"returnCode=$returnCode => calling plugin callback $orderFunction")
      orderFunction(order)
    }
  }

  @ForCpp
  private def orderStateTransitionToState(cppInternalValue: Long): String =
    orderStateTransitionToState(OrderNodeTransition.ofCppInternalValue(cppInternalValue)).string

  protected def obstacles: Set[NodeObstacle] = {
    import NodeObstacle._
    val builder = Set.newBuilder[NodeObstacle]
    action switch {
      case JobChainNodeAction.stop ⇒ builder += Stopping
    }
    if (!delay.isZero) {
      builder += Delaying(delay)
    }
    if (!jobSubsystem.contains(jobPath)) {
      builder += MissingJob(jobPath)
    }
    builder.result
  }

  private[kernel] final def orderCount: Int = orderQueue.size
}

object JobNode {
  private val logger = Logger(getClass)
}
