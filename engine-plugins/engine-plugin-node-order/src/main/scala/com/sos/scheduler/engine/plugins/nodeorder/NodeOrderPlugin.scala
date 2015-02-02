package com.sos.scheduler.engine.plugins.nodeorder

import com.sos.scheduler.engine.common.scalautil.Collections.implicits._
import com.sos.scheduler.engine.common.scalautil.Logger
import com.sos.scheduler.engine.common.scalautil.xmls.ScalaStax._
import com.sos.scheduler.engine.common.scalautil.xmls.ScalaXMLEventReader
import com.sos.scheduler.engine.common.scalautil.xmls.ScalaXMLEventReader._
import com.sos.scheduler.engine.data.jobchain.{JobChainPath, NodeKey}
import com.sos.scheduler.engine.data.message.MessageCode
import com.sos.scheduler.engine.data.order.{OrderId, OrderStepEndedEvent}
import com.sos.scheduler.engine.data.xmlcommands.OrderCommand
import com.sos.scheduler.engine.eventbus.{EventSourceEvent, SchedulerEventBus}
import com.sos.scheduler.engine.kernel.async.SchedulerThreadCallQueue
import com.sos.scheduler.engine.kernel.async.SchedulerThreadFutures.schedulerThreadFuture
import com.sos.scheduler.engine.kernel.log.PrefixLog
import com.sos.scheduler.engine.kernel.order.Order
import com.sos.scheduler.engine.kernel.order.jobchain.JobNode
import com.sos.scheduler.engine.kernel.plugin.{NamespaceXmlPlugin, Plugin, PluginXmlConfigurable}
import com.sos.scheduler.engine.kernel.scheduler.{SchedulerException, SchedulerXmlCommandExecutor}
import com.sos.scheduler.engine.plugins.nodeorder.NodeOrderPlugin.{parse, _}
import javax.inject.Inject
import org.w3c.dom
import scala.collection.mutable
import scala.util.control.NonFatal

/**
 * @author Joacim Zschimmer
 */
final class NodeOrderPlugin @Inject private(
  eventBus: SchedulerEventBus,
  xmlCommandExecutor: SchedulerXmlCommandExecutor,
  schedulerLogger: PrefixLog)
  (implicit callQueue: SchedulerThreadCallQueue)
  extends Plugin with NamespaceXmlPlugin {

  val xmlNamespace = "https://jobscheduler-plugins.sos-berlin.com/NodeOrderPlugin"

  private val nodeConfigurations = mutable.Map[NodeKey, NodeConfiguration]()

  override def onActivate() =
    eventBus.onHotEventSourceEvent[OrderStepEndedEvent] {
      case EventSourceEvent(_: OrderStepEndedEvent, order: Order) ⇒ onOrderStepEnded(order.nodeKey, order.id, order.parameters.toMap)
    }

  private def onOrderStepEnded(nodeKey: NodeKey, orderId: OrderId, parameters: Map[String, String]) = {
    for (conf ← nodeConfigurations.get(nodeKey)) {
      val addOrderCommand = OrderCommand(conf.jobChainPath orderKey orderId, parameters = parameters)
      schedulerThreadFuture {
        try xmlCommandExecutor executeXml addOrderCommand
        catch {
          case NonFatal(t) ⇒ schedulerLogger.error(commandFailedMessage(t))
        }
      }
    }
  }

  private def commandFailedMessage(t: Throwable): String = {
    val msg = if (t.isInstanceOf[SchedulerException]) t.getMessage else t.toString
    s"$CommandFailedCode Error when cloning order (ignored): $msg"
  }

  def attachPluginXmlConfigurable(obj: PluginXmlConfigurable, element: dom.Element) =
    obj match {
      case jobNode: JobNode ⇒
        val nodeKey = jobNode.nodeKey
        logger.debug(s"Attaching XML of $nodeKey")
        val nodeConfiguration = parseDocument(domElementToStaxSource(element))(parse)
        nodeConfigurations.insert(nodeKey → nodeConfiguration)
    }

  def detachPluginXmlConfigurable(obj: PluginXmlConfigurable) =
    obj match {
      case jobNode: JobNode ⇒
        logger.debug(s"Detaching ${jobNode.nodeKey}")
        nodeConfigurations -= jobNode.nodeKey
    }
}

object NodeOrderPlugin {
  private[nodeorder] val CommandFailedCode = MessageCode("NODE-ORDER-PLUGIN-100")
  private val logger = Logger(getClass)

  private def parse(eventReader: ScalaXMLEventReader): NodeConfiguration = {
    import eventReader._
    val jobChainPath = parseElement("add_order") {
      JobChainPath(attributeMap("job_chain"))
    }
    NodeConfiguration(jobChainPath)
  }

  private case class NodeConfiguration(jobChainPath: JobChainPath)
}
