package com.sos.scheduler.engine.kernel.plugin.jobchainnode

import com.sos.scheduler.engine.kernel.order.Order
import com.sos.scheduler.engine.kernel.order.jobchain.JobNode
import com.sos.scheduler.engine.kernel.plugin.NamespaceXmlPlugin
import javax.xml.stream.XMLEventReader

/**
 * A [[NamespaceXmlPlugin]] handling XML extensions of `&lt;job_chain_node>`.
 *
 * @author Joacim Zschimmer
 */
trait JobChainNodeNamespaceXmlPlugin extends NamespaceXmlPlugin {

  /**
   * Parses an XML extension in `&lt;on_return_code/>` and return a side-effecting order handler.
   */
  def parseOnReturnCodeXml(node: JobNode, xmlEventReader: XMLEventReader): Order ⇒ Unit
}
