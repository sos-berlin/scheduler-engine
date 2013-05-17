package com.sos.scheduler.engine.data.order

import com.sos.scheduler.engine.data.event.Event

trait OrderEvent extends Event {
  def orderKey: OrderKey
}
