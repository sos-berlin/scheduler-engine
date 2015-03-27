package com.sos.scheduler.engine.plugins.nodeorder

import com.sos.scheduler.engine.common.scalautil.ScalaUtils.implicits.ToStringFunction1
import com.sos.scheduler.engine.common.scalautil.xmls.ScalaXMLEventReader
import com.sos.scheduler.engine.data.filebased.AbsolutePath
import com.sos.scheduler.engine.data.jobchain.JobChainPath
import com.sos.scheduler.engine.data.message.MessageCode
import com.sos.scheduler.engine.data.order.{OrderId, OrderKey}
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
import org.scalactic.Requirements._
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

  val xmlNamespace = "https://jobscheduler-plugins.sos-berlin.com/NodeOrderPlugin"

  /**
   * Parses `&lt;job_chain_node>` XML extension `&lt;add_order job_chain='...'>`.
   *
   * @return The curried function `onReturnCode`, a `Order ⇒ Unit`
   */
  def parseOnReturnCodeXml(jobNode: JobNode, xmlEventReader: XMLEventReader): Order ⇒ Unit = {
    val eventReader = new ScalaXMLEventReader(xmlEventReader)
    import eventReader._
    val orderKeyPattern = parseElement("add_order") {
      val jobChainPath = JobChainPath(AbsolutePath.makeCompatibleAbsolute(defaultFolder = jobNode.jobChainPath.parent, path = attributeMap("job_chain")))
      val idPattern = attributeMap.getOrElse("id", "")
      OrderKeyPattern(jobChainPath, idPattern)
    }
    require(orderKeyPattern.idPattern.nonEmpty || orderKeyPattern.jobChainPath != jobNode.jobChainPath,
      s"${this.getClass.getName} <add_order job_chain='${orderKeyPattern.jobChainPath}'> must denote the own job_chain")
    onReturnCode(orderKeyPattern) _ withToString "NodeOrderPlugin.onResultCode"
  }

  private def onReturnCode(orderKeyPattern: OrderKeyPattern)(order: Order): Unit = {
    val addOrderCommand = OrderCommand(
      orderKey = orderKeyPattern.resolveWith(order.id),
      parameters = order.parameters.toMap)
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

  private case class OrderKeyPattern(jobChainPath: JobChainPath, idPattern: String) {
    def resolveWith(id: OrderId) = OrderKey(jobChainPath, resolveIdWith(id))

    private def resolveIdWith(id: OrderId) = idPattern match {
      case "" ⇒ id
      case _ ⇒ OrderId(idPattern.replace("${ORDER_ID}", id.string))
    }
  }
}
