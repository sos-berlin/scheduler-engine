package com.sos.scheduler.engine.kernel.order.jobchain

import com.google.common.io.Closer
import com.google.inject.Injector
import com.sos.scheduler.engine.common.inject.GuiceImplicits._
import com.sos.scheduler.engine.common.xml.XmlUtils.nodeListToSeq
import com.sos.scheduler.engine.cplusplus.runtime.Sister
import com.sos.scheduler.engine.cplusplus.runtime.annotation.ForCpp
import com.sos.scheduler.engine.data.jobchain.{NodeKey, JobChainNodeAction, JobChainNodePersistentState, JobChainPath, NodeOverview}
import com.sos.scheduler.engine.data.order.OrderState
import com.sos.scheduler.engine.kernel.cppproxy.NodeCI
import com.sos.scheduler.engine.kernel.plugin.{NamespaceXmlPlugin, PluginSubsystem, PluginXmlConfigurable}
import org.w3c.dom
import scala.collection.immutable

/** @author Joacim Zschimmer */
@ForCpp
abstract class Node extends Sister with PluginXmlConfigurable {

  implicit private val closer = Closer.create()
  protected val cppProxy: NodeCI
  protected def injector: Injector
  private var plugins: immutable.Seq[NamespaceXmlPlugin] = Nil

  def onCppProxyInvalidated(): Unit = {
    closer.close()
  }

  @ForCpp
  def processConfigurationDomElement(nodeElement: dom.Element): Unit = {
    val pluginSubsystem = injector.apply[PluginSubsystem]
    val elementPlugins = nodeListToSeq(nodeElement.getChildNodes) collect {
      case e: dom.Element ⇒ e → pluginSubsystem.pluginsByXmlNamespace(e.getNamespaceURI)
    }
    plugins = (elementPlugins flatMap { _._2 }).distinct
    for ((element, plugins) ← elementPlugins;
         plugin ← plugins) {
      plugin.attachPluginXmlConfigurable(this, element)
    }
  }

  final def persistentState =
    new JobChainNodePersistentState(jobChainPath, orderState, action)

  def overview: NodeOverview

  final def nodeKey = NodeKey(jobChainPath, orderState)

  final def jobChainPath =
    JobChainPath(cppProxy.job_chain_path)

  final def orderState =
    OrderState(cppProxy.string_order_state)

  final def nextState =
    OrderState(cppProxy.string_next_state)

  final def errorState =
    OrderState(cppProxy.string_error_state)

  final def action =
    JobChainNodeAction.ofCppName(cppProxy.string_action)

  final def action_=(o: JobChainNodeAction): Unit = {
    cppProxy.set_action_string(o.toCppName)
  }
}
