package com.sos.scheduler.engine.kernel.order.jobchain

import com.sos.scheduler.engine.common.scalautil.Logger
import com.sos.scheduler.engine.common.scalautil.xmls.ScalaXMLEventReader._
import com.sos.scheduler.engine.data.order.{KeepOrderStateTransition, OrderState, OrderStateTransition, ProceedingOrderStateTransition, SuccessOrderStateTransition}
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

  protected def orderState: OrderState

  protected def nextState: OrderState

  protected def errorState: OrderState

  private var returnCodeToOnReturnCode: PartialFunction[Int, OnReturnCode] = PartialFunction.empty

  /**
   * May not be called.
   * @param xmlSource The `&lt;job_chain_node>...&lt;/job_chain_node>` to be parsed
   * @param namespaceToOnReturnCodeParser PartialFunction, mapping an XML namespace to an XML parser,
   *                                      parsing the XML extension and returning a side-effecting function [[Order]] ⇒ [[Unit]]
   */
  def initializeWithNodeXml(xmlSource: XmlSource, namespaceToOnReturnCodeParser: PartialFunction[String, OnReturnCodeParser]) = {
    if (returnCodeToOnReturnCode ne PartialFunction.empty) throw new IllegalStateException
    returnCodeToOnReturnCode = parseNodeXml(xmlSource, namespaceToOnReturnCodeParser)
    logger.debug(s"$this: $returnCodeToOnReturnCode")
  }

  def orderStateTransitionToState(t: OrderStateTransition): OrderState =
    t match {
      case KeepOrderStateTransition ⇒ orderState
      case ProceedingOrderStateTransition(returnCode) ⇒
        returnCodeToOnReturnCode.lift(returnCode) match {
          case Some(OnReturnCode(_, Some(state), _)) ⇒ state
          case _ ⇒ if (t == SuccessOrderStateTransition) nextState else errorState
        }
    }

  /**
   * @return All functions [[Order]] ⇒ [[Unit]] to be executed when an order step ended with given `return_code`
   */
  def returnCodeToOrderFunctions(returnCode: Int): immutable.Seq[OrderFunction] = {
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

  private def parseNodeXml(xmlSource: XmlSource, namespaceToOnReturnCodeParser: PartialFunction[String, OnReturnCodeParser]) = {
    def parseNodeConfiguration(): NodeConfiguration = {
      parseDocument(xmlSource) { eventReader ⇒
        import eventReader._

        def parseOnReturnCodes(): immutable.Seq[OnReturnCode] =
          parseElement("on_return_codes") {
            parseEachRepeatingElement[OnReturnCode]("on_return_code") {
              val returnCodes = Set(attributeMap("return_code").toInt)
              val actions = forEachStartElement[ReturnCodeAction] {
                case "to_state" ⇒ parseToState()
                case unknown ⇒
                  val namespace = xmlEventReader.peek.asStartElement.getName.getNamespaceURI
                  namespaceToOnReturnCodeParser.lift(namespace) match {
                    case Some(parse) ⇒
                      Callback(parse(xmlEventReader))
                    case None ⇒
                      logger.debug(s"Ignoring unknown XML namespace: ${xmlEventReader.peek.asStartElement.getName}")
                      ignoreElement()
                      NoAction
                  }
              }
              OnReturnCode(returnCodes, actions.option[ToState] map { _.state }, actions.byClass[Callback])
            }
          }

        def parseToState(): ToState =
          parseElement("to_state") {
            ToState(OrderState(attributeMap("state")))
          }

        parseElement("job_chain_node") {
          attributeMap.ignoreUnread()
          case class OnReturnCodes(onReturnCodes: immutable.Seq[OnReturnCode])
          val elementMap = forEachStartElement {
            case "on_return_codes" ⇒ OnReturnCodes(parseOnReturnCodes())
            case unknown ⇒ ignoreElement(); Unit
          }
          NodeConfiguration(elementMap.byClass[OnReturnCodes] flatMap { _.onReturnCodes })
        }
      }
    }

    parseNodeConfiguration() match {
      case NodeConfiguration(onReturnCodes) ⇒ (onReturnCodes flatMap { o ⇒ o.returnCodes map { _ → o } }).toMap
    }
  }

  private[jobchain] type ValueToState = PartialFunction[Int, OrderState]

  private case class NodeConfiguration(onReturnCodes: immutable.Seq[OnReturnCode])

  private case class OnReturnCode(returnCodes: Set[Int], toStateOption: Option[OrderState], callbacks: immutable.Seq[Callback])

  private sealed trait ReturnCodeAction

  private case class ToState(state: OrderState) extends ReturnCodeAction

  private case class Callback(orderFunction: OrderFunction) extends ReturnCodeAction

  private case object NoAction extends ReturnCodeAction
}
