package com.sos.scheduler.engine.kernel.order.jobchain

import com.google.inject.Injector
import com.sos.scheduler.engine.common.scalautil.xmls.ScalaStax._
import com.sos.scheduler.engine.common.scalautil.xmls.ScalaXMLEventReader._
import com.sos.scheduler.engine.cplusplus.runtime.annotation.ForCpp
import com.sos.scheduler.engine.cplusplus.runtime.{Sister, SisterType}
import com.sos.scheduler.engine.data.job.JobPath
import com.sos.scheduler.engine.data.jobchain.JobNodeOverview
import com.sos.scheduler.engine.data.order.{KeepOrderStateTransition, OrderState, OrderStateTransition, ProceedingOrderStateTransition, SuccessOrderStateTransition}
import com.sos.scheduler.engine.kernel.cppproxy.Job_nodeC
import com.sos.scheduler.engine.kernel.job.Job
import com.sos.scheduler.engine.kernel.order.jobchain.JobNode._
import com.sos.scheduler.engine.kernel.scheduler.HasInjector
import org.w3c.dom

@ForCpp
final class JobNode(
  protected val cppProxy: Job_nodeC,
  protected val injector: Injector)
extends OrderQueueNode {

  private var nodeConfiguration: NodeConfiguration = null

  override def processConfigurationDomElement(nodeElement: dom.Element) = {
    nodeConfiguration = parseNodeConfiguration(domElementToStaxSource(nodeElement))
    super.processConfigurationDomElement(nodeElement)
  }

  @ForCpp
  def orderStateTransitionToState(cppInternalValue: Long): String =
    orderStateTransitionToState(OrderStateTransition.ofCppInternalValue(cppInternalValue)).string

  private def orderStateTransitionToState(t: OrderStateTransition): OrderState =
    t match {
      case KeepOrderStateTransition ⇒ orderState
      case ProceedingOrderStateTransition(resultValue) ⇒
        nodeConfiguration.valueToState.lift(resultValue) match {
          case Some(state) ⇒ state
          case None ⇒ if (t == SuccessOrderStateTransition) nextState else errorState
        }
    }

  override def overview = JobNodeOverview(
    orderState = orderState,
    nextState = nextState,
    errorState = errorState,
    orderCount = orderCount,
    action = action,
    jobPath = jobPath)

  def jobPath: JobPath = JobPath(cppProxy.job_path)

  def getJob: Job = cppProxy.job.getSister
}


object JobNode {
  final class Type extends SisterType[JobNode, Job_nodeC] {
    def sister(proxy: Job_nodeC, context: Sister): JobNode = {
      val injector = context.asInstanceOf[HasInjector].injector
      new JobNode(proxy, injector)
    }
  }

  private def parseNodeConfiguration(source: javax.xml.transform.Source): NodeConfiguration =
    parseDocument(source) { eventReader ⇒
      import eventReader._
      var valueToState: Option[ValueToState] = None
      parseElement("job_chain_node") {
        attributeMap.ignoreUnread()
        forEachStartElement {
          case "on_result_values" ⇒
            parseElement() {
              val maps = parseEachRepeatingElement("on_result_value") {
                val resultValues = List(attributeMap("result_value").toInt)
                val orderState = parseElement("to_state") {
                  OrderState(attributeMap("state"))
                }
                resultValues map { _ → orderState }
              }
              valueToState = Some((maps reduce { _ ++ _ }).toMap)
            }
          case unknown ⇒
            ignoreElement()
            PartialFunction.empty
        }
      }
      NodeConfiguration(valueToState getOrElse PartialFunction.empty)
    }

  private type ValueToState = PartialFunction[Int, OrderState]

  private case class NodeConfiguration(valueToState: ValueToState)
}
