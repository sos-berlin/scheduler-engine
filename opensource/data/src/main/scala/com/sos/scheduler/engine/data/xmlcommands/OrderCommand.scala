package com.sos.scheduler.engine.data.xmlcommands

import com.sos.scheduler.engine.data.order.{OrderState, OrderKey}

final case class OrderCommand(orderKey: OrderKey, state: Option[OrderState] = None) extends XmlCommand {
  def xmlElem = <order
    job_chain={orderKey.jobChainPathString}
    id={orderKey.idString}
    state={(state map { _.string }).orNull}/>
}
