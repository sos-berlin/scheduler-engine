package com.sos.scheduler.engine.eventbus

import com.sos.jobscheduler.data.event.NoKeyEvent

final case class EventHandlerFailedEvent(call: Call, throwable: Throwable)
extends NoKeyEvent
