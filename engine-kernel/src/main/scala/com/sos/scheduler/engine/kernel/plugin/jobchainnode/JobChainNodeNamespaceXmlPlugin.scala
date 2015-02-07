package com.sos.scheduler.engine.kernel.plugin.jobchainnode

import com.sos.scheduler.engine.kernel.order.Order
import com.sos.scheduler.engine.kernel.order.jobchain.JobNode
import com.sos.scheduler.engine.kernel.plugin.NamespaceXmlPlugin
import javax.xml.stream.XMLEventReader

/**
 * @author Joacim Zschimmer
 */
trait JobChainNodeNamespaceXmlPlugin extends NamespaceXmlPlugin {

  def parseOnReturnCodeXml(node: JobNode, xmlEventReader: XMLEventReader): Order ⇒ Unit
}
