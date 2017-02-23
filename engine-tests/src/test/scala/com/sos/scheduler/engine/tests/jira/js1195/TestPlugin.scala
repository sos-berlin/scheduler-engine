package com.sos.scheduler.engine.tests.jira.js1195

import com.google.inject.Injector
import com.sos.jobscheduler.common.scalautil.Logger
import com.sos.jobscheduler.common.scalautil.xmls.ScalaXMLEventReader
import com.sos.scheduler.engine.common.xml.CppXmlUtils.domElementToStaxSource
import com.sos.jobscheduler.data.event.KeyedEvent
import com.sos.scheduler.engine.data.jobchain.{JobChainPath, NodeKey}
import com.sos.scheduler.engine.data.order.{OrderKey, OrderStepEnded}
import com.sos.scheduler.engine.data.xmlcommands.OrderCommand
import com.sos.scheduler.engine.eventbus.SchedulerEventBus
import com.sos.scheduler.engine.kernel.order.jobchain.JobNode
import com.sos.scheduler.engine.kernel.plugin.{AttachableNamespaceXmlPlugin, PluginXmlConfigurable}
import com.sos.scheduler.engine.kernel.scheduler.{HasInjector, SchedulerXmlCommandExecutor}
import com.sos.scheduler.engine.test.SchedulerTestUtils.orderDetailed
import com.sos.scheduler.engine.tests.jira.js1195.TestPlugin._
import javax.inject.Inject
import org.scalactic.Requirements._
import org.w3c.dom
import scala.collection.mutable

/**
 * @author Joacim Zschimmer
 */
final class TestPlugin @Inject private(
  eventBus: SchedulerEventBus,
  xmlCommandExecutor: SchedulerXmlCommandExecutor,
  injector: Injector)
extends AttachableNamespaceXmlPlugin {

  private implicit val hasInjector = HasInjector(injector)
  private val nodeConfigurations = mutable.Map[NodeKey, NodeConfiguration]()

  val xmlNamespace = "http://example.com/TestPlugin"

  override def onActivate(): Unit = {
    eventBus.onHot[OrderStepEnded] {
      case KeyedEvent(orderKey: OrderKey, _) ⇒
        val order = orderDetailed(orderKey)
        for (conf ← nodeConfigurations.get(order.overview.nodeKey)) {
          val command = OrderCommand(
            OrderKey(conf.jobChainPath, orderKey.id),
            parameters = order.variables)
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
