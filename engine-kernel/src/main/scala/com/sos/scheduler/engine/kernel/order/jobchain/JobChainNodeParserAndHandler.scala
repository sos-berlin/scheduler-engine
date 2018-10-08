package com.sos.scheduler.engine.kernel.order.jobchain

import com.sos.scheduler.engine.common.scalautil.Logger
import com.sos.scheduler.engine.common.scalautil.xmls.ScalaXMLEventReader._
import com.sos.scheduler.engine.data.job.ReturnCode
import com.sos.scheduler.engine.data.jobchain.NodeId
import com.sos.scheduler.engine.data.order.OrderNodeTransition
import com.sos.scheduler.engine.kernel.order.Order
import com.sos.scheduler.engine.kernel.order.jobchain.JobChainNodeParserAndHandler._
import javax.xml.stream.XMLEventReader
import javax.xml.transform.{Source ⇒ XmlSource}
import scala.collection.immutable

/**
 * Parses and handles `&lt;on_return_codes>` in `&lt;job_chain_node>`.
 *
 * @author Joacim Zschimmer
 */
private[jobchain] trait JobChainNodeParserAndHandler {

  protected[kernel] def nodeId: NodeId

  protected def nextNodeId: NodeId

  protected def errorNodeId: NodeId

  private var returnCodeToOnReturnCode = PartialFunction.empty[ReturnCode, OnReturnCode]

  /**
   * May not be called.
   * @param xmlSource The `&lt;job_chain_node>...&lt;/job_chain_node>` to be parsed
   * @param namespaceToOnReturnCodeParser Function, mapping an XML namespace to an XML parser (or None when no XML parser is available),
   *                                      parsing the XML extension and returning a side-effecting function [[Order]] ⇒ `Unit`.
   */
  def initializeWithNodeXml(xmlSource: XmlSource, namespaceToOnReturnCodeParser: String ⇒ Option[OnReturnCodeParser]) = {
    if (returnCodeToOnReturnCode ne PartialFunction.empty) throw new IllegalStateException
    returnCodeToOnReturnCode = parseNodeXml(xmlSource, namespaceToOnReturnCodeParser)
  }

  def orderStateTransitionToState(t: OrderNodeTransition): Option[NodeId] =
    t match {
      case OrderNodeTransition.Keep ⇒
        Some(nodeId)

      case OrderNodeTransition.Proceeding(returnCode) ⇒
        returnCodeToOnReturnCode.lift(returnCode) match {
          case Some(OnReturnCode(_, Some(nodeId), _)) ⇒  // <to_state>
            Some(nodeId)

          case _ ⇒
            if (t == OrderNodeTransition.Success) Some(nextNodeId) else None/*error*/
        }
    }

  /**
   * @return All functions [[Order]] ⇒ `Unit` to be executed when an order step ended with given `return_code`
   */
  def returnCodeToOrderFunctions(returnCode: ReturnCode): immutable.Seq[OrderFunction] = {
    returnCodeToOnReturnCode.lift(returnCode) match {
      case Some(OnReturnCode(_, _, callbacks)) ⇒ callbacks map { _.orderFunction }
      case _ ⇒ Nil
    }
  }
}

private[jobchain] object JobChainNodeParserAndHandler {
  type OnReturnCodeParser = XMLEventReader ⇒ OrderFunction

  /** Side-effecting function, called with order when the result value of a jobchain step matches. */
  type OrderFunction = Order ⇒ Unit

  private val logger = Logger(getClass)

  private def parseNodeXml(xmlSource: XmlSource, namespaceToOnReturnCodeParser: String ⇒ Option[OnReturnCodeParser]): PartialFunction[ReturnCode, OnReturnCode] = {
    def parseNodeConfiguration(): NodeConfiguration = {
      parseDocument(xmlSource) { eventReader ⇒
        import eventReader._

        def parseOnReturnCodes(): immutable.Seq[OnReturnCode] =
          parseElement("on_return_codes") {
            parseEachRepeatingElement[OnReturnCode]("on_return_code") {
              val returnCodes = RangeSet(attributeMap("return_code"))
              val actions = forEachStartElement[ReturnCodeAction] {
                case "to_state" ⇒ parseToState()
                case _ ⇒
                  val namespace = xmlEventReader.peek.asStartElement.getName.getNamespaceURI
                  namespaceToOnReturnCodeParser(namespace) match {
                    case Some(parse) ⇒
                      Callback(parse(xmlEventReader))
                    case None ⇒
                      logger.debug(s"Ignoring unknown XML namespace: ${xmlEventReader.peek.asStartElement.getName}")
                      ignoreElement()
                      NoAction
                  }
              }
              OnReturnCode(returnCodes, actions.option[ToNodeId] map { _.nodeId }, actions.byClass[Callback])
            }
          }

        def parseToState(): ToNodeId =
          parseElement("to_state") {
            ToNodeId(NodeId(attributeMap("state")))
          }

        parseElement("job_chain_node") {
          attributeMap.ignoreUnread()
          case class OnReturnCodes(onReturnCodes: immutable.Seq[OnReturnCode])
          val elementMap = forEachStartElement {
            case "on_return_codes" ⇒ OnReturnCodes(parseOnReturnCodes())
            case _ ⇒ ignoreElement(); Unit
          }
          NodeConfiguration(elementMap.byClass[OnReturnCodes] flatMap { _.onReturnCodes })
        }
      }
    }

    parseNodeConfiguration() match {
      case NodeConfiguration(onReturnCodes) ⇒ Function.unlift(returnCodeToOnReturnCode(onReturnCodes))
    }
  }

  private[jobchain] type ValueToNodeId = PartialFunction[Int, NodeId]

  private case class NodeConfiguration(onReturnCodes: immutable.Seq[OnReturnCode])

  private case class OnReturnCode(returnCodes: RangeSet, toNodeIdOption: Option[NodeId], callbacks: immutable.Seq[Callback])

  private def returnCodeToOnReturnCode(onReturnCodes: immutable.Seq[OnReturnCode])(returnCode: ReturnCode): Option[OnReturnCode] =
    onReturnCodes collectFirst { case o if o.returnCodes contains returnCode.toInt ⇒ o }

  private sealed trait ReturnCodeAction

  private case class ToNodeId(nodeId: NodeId) extends ReturnCodeAction

  private case class Callback(orderFunction: OrderFunction) extends ReturnCodeAction

  private case object NoAction extends ReturnCodeAction
}
