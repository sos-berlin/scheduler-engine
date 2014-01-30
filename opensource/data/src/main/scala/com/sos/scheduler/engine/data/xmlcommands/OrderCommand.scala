package com.sos.scheduler.engine.data.xmlcommands

import com.sos.scheduler.engine.data.order.{OrderState, OrderKey}

final case class OrderCommand(
    orderKey: OrderKey,
    state: Option[OrderState] = None,
    title: Option[String] = None,
    xmlChildren: xml.NodeSeq = Nil)
extends XmlCommand {

  def xmlElem = <order
    job_chain={orderKey.jobChainPath.string}
    id={orderKey.id.string}
    state={(state map { _.string }).orNull}
    title={title.orNull}>{xmlChildren}</order>
}
