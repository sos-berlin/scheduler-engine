package com.sos.scheduler.engine.data.xmlcommands

import ModifyOrderCommand._
import com.sos.scheduler.engine.data.order.OrderKey
//import org.joda.time.DateTimeZone.UTC
//import org.joda.time.ReadableInstant
//import org.joda.time.format.DateTimeFormat

final case class ModifyOrderCommand(
    orderKey: OrderKey,
    action: Option[Action.Value] = None,
    at: Option[At] = None)
extends XmlCommand {

  def xmlElem = <modify_order
    job_chain={orderKey.jobChainPathString}
    order={orderKey.idString}
    action={(action map { _.toString }).orNull}
    at={(at map { _.string }).orNull}
    />
}


object ModifyOrderCommand {
//  private val yyyymmddhhmmssFormatter = DateTimeFormat forPattern "yyyy-MM-dd HH:mm:ss'Z'"

  object Action extends Enumeration {
    val reset = Value
  }

  sealed trait At {
    def string: String
  }

  case object NowAt extends At {
    def string = "now"
  }

//  case object NowPlus(duration: Duration) extends At {
//    def string = s"now+" + ???
//  }

//  case class InstantAt(instant: ReadableInstant) extends At {
//    def string = yyyymmddhhmmssFormatter print (instant.toInstant toDateTime UTC)
//  }
}