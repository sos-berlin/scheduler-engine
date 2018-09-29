package com.sos.scheduler.engine.eventbus

import com.google.inject.ImplementedBy
import com.sos.scheduler.engine.data.event.KeyedEvent
import com.sos.scheduler.engine.data.events.custom.CustomEvent
import com.sos.scheduler.engine.kernel.async.SchedulerThreadCallQueue
import com.sos.scheduler.engine.kernel.async.SchedulerThreadFutures.inSchedulerThread
import javax.inject.Inject

/**
  * @author Joacim Zschimmer
  */
@ImplementedBy(classOf[StandardEventPublisher])
trait EventPublisher
{
  def publishCustomEvent[E <: CustomEvent](keyedEvent: KeyedEvent[E]): Unit
}

final class StandardEventPublisher @Inject private(eventBus: SchedulerEventBus)(implicit schedulerThreadCallQueue: SchedulerThreadCallQueue)
extends EventPublisher
{
  def publishCustomEvent[E <: CustomEvent](keyedEvent: KeyedEvent[E]): Unit =
    inSchedulerThread {
      eventBus.publish(keyedEvent)
    }
}
