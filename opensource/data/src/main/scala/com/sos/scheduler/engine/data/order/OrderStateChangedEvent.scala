package com.sos.scheduler.engine.data.order

import com.sos.scheduler.engine.data.event.AbstractEvent

final case class OrderStateChangedEvent(orderKey: OrderKey, previousState: OrderState)
extends AbstractEvent  // Für @ForCpp: Funktionen müssen eine Klasse, kein Interface liefern
with OrderEvent
