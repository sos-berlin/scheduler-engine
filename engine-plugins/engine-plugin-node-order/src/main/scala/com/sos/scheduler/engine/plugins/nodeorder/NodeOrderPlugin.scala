package com.sos.scheduler.engine.plugins.nodeorder

import com.sos.jobscheduler.base.utils.ScalaUtils.implicits.ToStringFunction1
import com.sos.jobscheduler.common.scalautil.xmls.ScalaXMLEventReader
import com.sos.jobscheduler.common.xml.VariableSets
import com.sos.jobscheduler.data.folder.FolderPath
import com.sos.jobscheduler.data.message.MessageCode
import com.sos.jobscheduler.data.order.OrderId
import com.sos.scheduler.engine.data.jobchain.JobChainPath
import com.sos.scheduler.engine.data.order.OrderKey
import com.sos.scheduler.engine.data.xmlcommands.OrderCommand
import com.sos.scheduler.engine.eventbus.SchedulerEventBus
import com.sos.scheduler.engine.kernel.async.SchedulerThreadCallQueue
import com.sos.scheduler.engine.kernel.async.SchedulerThreadFutures.schedulerThreadFuture
import com.sos.scheduler.engine.kernel.log.PrefixLog
import com.sos.scheduler.engine.kernel.order.Order
import com.sos.scheduler.engine.kernel.order.jobchain.JobNode
import com.sos.scheduler.engine.kernel.plugin.jobchainnode.JobChainNodeNamespaceXmlPlugin
import com.sos.scheduler.engine.kernel.scheduler.{SchedulerException, SchedulerXmlCommandExecutor}
import com.sos.scheduler.engine.plugins.nodeorder.NodeOrderPlugin._
import javax.inject.Inject
import javax.xml.stream.XMLEventReader
import scala.util.control.NonFatal

/**
 * Plugin for the job-chain node XML extension `&lt;add_order job_chain='...'>` to start add an order
 * with equal order ID and a copy of the order's parameters in a different job-chain.
 * Any error when adding the order is logged to the main log and ignored.
 *
 * @see https://change.sos-berlin.com/browse/JS-1193
 * @author Joacim Zschimmer
 */
final class NodeOrderPlugin @Inject private(
  eventBus: SchedulerEventBus,
  xmlCommandExecutor: SchedulerXmlCommandExecutor,
  schedulerLogger: PrefixLog)
  (implicit callQueue: SchedulerThreadCallQueue)
extends JobChainNodeNamespaceXmlPlugin {

  val xmlNamespace = XmlNamespace

  /**
   * Parses `&lt;job_chain_node>` XML extension `&lt;add_order job_chain='...'>`.
   *
   * @return The curried function `onReturnCode`, a `Order ⇒ Unit`
   */
  def parseOnReturnCodeXml(jobNode: JobNode, xmlEventReader: XMLEventReader): Order ⇒ Unit = {
    val eventReader = new ScalaXMLEventReader(xmlEventReader)
    import eventReader._
    val addOrder = parseElement("add_order") {
      val jobChainPath = FolderPath.parentOf(jobNode.jobChainPath) resolve[JobChainPath] attributeMap("job_chain")
      val idPattern = attributeMap.getOrElse("id", "")
      if (jobChainPath == jobNode.jobChainPath && idPattern.isEmpty)
        throw new IllegalArgumentException(s"${this.getClass.getName} <add_order job_chain='$jobChainPath'> without attribute id= must not denote the own job_chain")
      val elementMap = forEachStartElement {
        case ParamsElementName ⇒ VariableSets.parseXml(eventReader, "params", s"{$XmlNamespace}param")
      }
      AddOrder(OrderKeyPattern(jobChainPath, idPattern), variables = elementMap.option[Map[String,String]](ParamsElementName) getOrElse Map())
    }
    onReturnCode(addOrder) _ withToString "NodeOrderPlugin.onResultCode"
  }

  private def onReturnCode(addOrder: AddOrder)(order: Order): Unit = {
    val addOrderCommand = OrderCommand(
      orderKey = addOrder.orderKeyPattern.resolveWith(order.id),
      parameters = order.variables ++ addOrder.variables)
    schedulerThreadFuture {
      try xmlCommandExecutor executeXml addOrderCommand
      catch {
        case NonFatal(t) ⇒ schedulerLogger.error(commandFailedMessage(t))
      }
    }
  }

  private def commandFailedMessage(t: Throwable): String = {
    val msg = if (t.isInstanceOf[SchedulerException]) t.getMessage else t.toString
    s"$CommandFailedCode Error when cloning order (ignored): $msg"
  }
}

object NodeOrderPlugin {
  private[nodeorder] val CommandFailedCode = MessageCode("NODE-ORDER-PLUGIN-100")

  private val XmlNamespace = "https://jobscheduler-plugins.sos-berlin.com/NodeOrderPlugin"
  private val ParamsElementName = s"{$XmlNamespace}params"

  private case class AddOrder(orderKeyPattern: OrderKeyPattern, variables: Map[String, String])

  private case class OrderKeyPattern(jobChainPath: JobChainPath, idPattern: String) {
    def resolveWith(id: OrderId) = OrderKey(jobChainPath, resolveIdWith(id))

    private def resolveIdWith(id: OrderId) = idPattern match {
      case "" ⇒ id
      case _ ⇒ OrderId(idPattern.replace("${ORDER_ID}", id.string))
    }
  }
}
