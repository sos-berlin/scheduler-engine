package com.sos.scheduler.engine.kernel.order.jobchain

import com.google.inject.Injector
import com.sos.scheduler.engine.common.guice.GuiceImplicits._
import com.sos.scheduler.engine.common.scalautil.{HasCloser, SetOnce}
import com.sos.scheduler.engine.common.xml.XmlUtils.nodeListToSeq
import com.sos.scheduler.engine.cplusplus.runtime.Sister
import com.sos.scheduler.engine.cplusplus.runtime.annotation.ForCpp
import com.sos.scheduler.engine.data.jobchain.{JobChainNodeAction, JobChainNodePersistentState, JobChainPath, NodeKey, NodeOverview}
import com.sos.scheduler.engine.data.order.OrderState
import com.sos.scheduler.engine.kernel.cppproxy.NodeCI
import com.sos.scheduler.engine.kernel.plugin.{AttachableNamespaceXmlPlugin, PluginSubsystem, PluginXmlConfigurable}
import org.w3c.dom

/**
 * @author Joacim Zschimmer
 */
@ForCpp
abstract class Node extends Sister with PluginXmlConfigurable with HasCloser {

  protected def injector: Injector

  def overview: NodeOverview

  protected val cppProxy: NodeCI

  private val orderStateOnce = new SetOnce[OrderState]

  def onCppProxyInvalidated() = close()

  @ForCpp
  def processConfigurationDomElement(nodeElement: dom.Element): Unit = {
    val elementPluginOption = nodeListToSeq(nodeElement.getChildNodes) collect {
      case e: dom.Element ⇒ e → injector.instance[PluginSubsystem].xmlNamespaceToPlugins[AttachableNamespaceXmlPlugin](e.getNamespaceURI)
    }
    for ((element, plugins) ← elementPluginOption; plugin ← plugins) {
      plugin.attachPluginXmlConfigurable(this, element)
    }
  }

  final def persistentState = new JobChainNodePersistentState(jobChainPath, orderState, action)

  final def nodeKey = NodeKey(jobChainPath, orderState)

  final def jobChainPath = JobChainPath(cppProxy.job_chain_path)

  final def orderState = orderStateOnce getOrUpdate OrderState(cppProxy.string_order_state)

  final def nextState = OrderState(cppProxy.string_next_state)

  final def errorState = OrderState(cppProxy.string_error_state)

  final def action = JobChainNodeAction.ofCppName(cppProxy.string_action)

  final def action_=(o: JobChainNodeAction): Unit = cppProxy.set_action_string(o.toCppName)
}
