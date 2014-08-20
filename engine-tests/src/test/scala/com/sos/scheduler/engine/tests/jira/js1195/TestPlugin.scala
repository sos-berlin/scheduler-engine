package com.sos.scheduler.engine.tests.jira.js1195

import com.sos.scheduler.engine.common.scalautil.Logger
import com.sos.scheduler.engine.common.scalautil.xml.ScalaStax._
import com.sos.scheduler.engine.common.scalautil.xml.ScalaXMLEventReader
import com.sos.scheduler.engine.data.jobchain.{JobChainPath, NodeKey}
import com.sos.scheduler.engine.data.order.{OrderKey, OrderStepEndedEvent}
import com.sos.scheduler.engine.data.xmlcommands.OrderCommand
import com.sos.scheduler.engine.eventbus.{EventSourceEvent, SchedulerEventBus}
import com.sos.scheduler.engine.kernel.order.Order
import com.sos.scheduler.engine.kernel.order.jobchain.JobNode
import com.sos.scheduler.engine.kernel.plugin.{NamespaceXmlPlugin, Plugin, PluginXmlConfigurable}
import com.sos.scheduler.engine.kernel.scheduler.SchedulerXmlCommandExecutor
import com.sos.scheduler.engine.tests.jira.js1195.TestPlugin._
import javax.inject.Inject
import org.scalactic.Requirements._
import org.w3c.dom
import scala.collection.mutable

/**
 * @author Joacim Zschimmer
 */
final class TestPlugin @Inject private(eventBus: SchedulerEventBus, xmlCommandExecutor: SchedulerXmlCommandExecutor)
extends Plugin with NamespaceXmlPlugin {

  private val nodeConfigurations = mutable.Map[NodeKey, NodeConfiguration]()

  val xmlNamespace = "http://example.com/TestPlugin"

  override def onActivate(): Unit = {
    eventBus.onHotEventSourceEvent[OrderStepEndedEvent] {
      case EventSourceEvent(e: OrderStepEndedEvent, order: Order) ⇒
        for (conf ← nodeConfigurations.get(order.nodeKey)) {
          val command = OrderCommand(
            OrderKey(conf.jobChainPath, order.id),
            parameters = order.parameters.toMap)
          xmlCommandExecutor executeXml command.xmlString //TODO Fehlerbehandlung?
        }
    }
  }

  def attachPluginXmlConfigurable(obj: PluginXmlConfigurable, element: dom.Element) =
    obj match {
      case jobNode: JobNode ⇒
        val nodeKey = jobNode.nodeKey
        logger.debug(s"Attaching XML of $nodeKey")
        val orderStepEndConfiguration = ScalaXMLEventReader.parseDocument(domElementToStaxSource(element))(parse)
        require(!(nodeConfigurations contains nodeKey))
        nodeConfigurations += nodeKey → orderStepEndConfiguration
    }

  def detachPluginXmlConfigurable(obj: PluginXmlConfigurable) =
    obj match {
      case jobNode: JobNode ⇒
        logger.debug(s"Detaching ${jobNode.nodeKey}")
        nodeConfigurations -= jobNode.nodeKey
    }
}

private object TestPlugin {
  private val logger = Logger(getClass)

  private def parse(eventReader: ScalaXMLEventReader): NodeConfiguration = {
    import eventReader._
    val jobChainPath = parseElement("clone_order") {
      JobChainPath(attributeMap("job_chain"))
    }
    NodeConfiguration(jobChainPath)
  }

  private case class NodeConfiguration(jobChainPath: JobChainPath)
}
