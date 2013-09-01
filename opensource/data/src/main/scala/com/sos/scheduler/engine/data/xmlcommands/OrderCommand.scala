package com.sos.scheduler.engine.data.xmlcommands

import com.sos.scheduler.engine.data.order.OrderKey

final case class OrderCommand(orderKey: OrderKey) extends XmlCommand {
  def xmlElem = <order job_chain={orderKey.jobChainPathString} id={orderKey.idString}/>
}
