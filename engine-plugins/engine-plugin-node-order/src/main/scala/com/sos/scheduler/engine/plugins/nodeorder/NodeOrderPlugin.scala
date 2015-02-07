package com.sos.scheduler.engine.plugins.nodeorder

import com.sos.scheduler.engine.common.scalautil.xmls.ScalaXMLEventReader
import com.sos.scheduler.engine.data.jobchain.JobChainPath
import com.sos.scheduler.engine.data.message.MessageCode
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
 * @author Joacim Zschimmer
 */
final class NodeOrderPlugin @Inject private(
  eventBus: SchedulerEventBus,
  xmlCommandExecutor: SchedulerXmlCommandExecutor,
  schedulerLogger: PrefixLog)
  (implicit callQueue: SchedulerThreadCallQueue)
extends JobChainNodeNamespaceXmlPlugin {

  val xmlNamespace = "https://jobscheduler-plugins.sos-berlin.com/NodeOrderPlugin"

  def parseOnReturnCodeXml(jobNode: JobNode, xmlEventReader: XMLEventReader): Order ⇒ Unit = {
    val eventReader = new ScalaXMLEventReader(xmlEventReader)
    import eventReader._
    val jobChainPath = parseElement("add_order") {
      JobChainPath(attributeMap("job_chain"))
    }
    require(jobChainPath != jobNode.jobChainPath, s"${this.getClass.getName} <add_order job_chain='$jobChainPath'> must denote the own job_chain")
    onReturnCode(jobChainPath)
  }

  private def onReturnCode(jobChainPath: JobChainPath)(order: Order): Unit = {
    val addOrderCommand = OrderCommand(jobChainPath orderKey order.id, parameters = order.parameters.toMap)
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
}
