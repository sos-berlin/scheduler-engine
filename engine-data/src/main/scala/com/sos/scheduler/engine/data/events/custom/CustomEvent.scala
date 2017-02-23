package com.sos.scheduler.engine.data.events.custom

import com.sos.jobscheduler.base.generic.IsString
import com.sos.jobscheduler.base.sprayjson.typed.{Subtype, TypedJsonFormat}
import com.sos.jobscheduler.data.event.Event

/**
  * @author Joacim Zschimmer
  */
trait CustomEvent extends Event {
  type Key = CustomEvent.Key
}

object CustomEvent {
  final case class Key(string: String) extends IsString
  object Key extends IsString.Companion[Key]

  implicit val OrderEventJsonFormat = TypedJsonFormat[CustomEvent](
    Subtype[VariablesCustomEvent])
}
