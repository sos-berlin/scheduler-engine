package com.sos.scheduler.engine.playground.plugins.jobnet

import com.sos.scheduler.engine.data.order.OrderState
import com.sos.scheduler.engine.playground.plugins.jobnet.node.{JobNode, Node}
import com.sos.scheduler.engine.playground.plugins.jobnet.end.EndNode
import scala.collection.immutable
import com.sos.scheduler.engine.playground.plugins.jobnet.scheduler.{JobChain, Order}

final class RawJobNet(jobChain: JobChain, val startState: OrderState, val nodeMap: Map[OrderState, Node]) {
  import RawJobNet._

  def onOrderStepEnded(order: Order) {
    require(order.jobChain eq jobChain)
    node(order.state).entrance.apply(order)
  }

  def toSchedulerJobChainXml: String =
    <job_chain>{nodesWithStartNodeFirst map nodeToXmlElem}</job_chain>.toString()

  def nodesWithStartNodeFirst =
    immutable.Seq(startNode) ++ (nodeMap.values filter { _ ne startNode })

  def startNode = node(startState)

  def node(o: OrderState) = nodeMap(o)
}

object RawJobNet {
  def apply(jobChain: JobChain, startState: OrderState, nodes: Set[Node]) =
    new RawJobNet(jobChain, startState, (nodes map { o => o.entrance.state -> o}).toMap)

  private def nodeToXmlElem(n: Node) = n match {
    case n: JobNode => <job_chain_node state={n.entrance.state.string} job={n.jobPath}/>
    case n: EndNode => <job_chain_node.end state={n.entrance.state.string}/>
  }
}
