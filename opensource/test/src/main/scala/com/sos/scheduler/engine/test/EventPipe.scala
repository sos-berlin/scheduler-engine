package com.sos.scheduler.engine.test

import java.util.concurrent.LinkedBlockingQueue
import java.util.concurrent.TimeUnit
import com.sos.scheduler.engine.main.event.TerminatedEvent
import com.sos.scheduler.engine.eventbus._
import com.sos.scheduler.engine.kernel.util.Time

class EventPipe extends EventHandlerAnnotated {
    import EventPipe._
    private final val queue = new LinkedBlockingQueue[Event]
    @volatile private var lastEventAdded = false

    @EventHandler def add(e: Event) {
        lastEventAdded |= lastEventClass isAssignableFrom e.getClass
        queue.add(e)
    }

    def expectEvent[E <: Event](t: Time)(predicate: E => Boolean)(implicit m: ClassManifest[E]): E = {
        val eventClass = m.erasure
        def className = eventClass.getSimpleName
        while (true) {
            val eventOption = tryPoll(t)
            if (lastEventAdded) throw new RuntimeException("Expected Event '"+className+"' has not arrived before "+lastEventClass.getName+" has arrived")
            eventOption match {
                case None => throw new RuntimeException("Expected Event '"+className+"' has not arrived within "+t)
                case Some(event: Event) if eventClass isAssignableFrom event.getClass =>
                    val e = event.asInstanceOf[E]
                    if (predicate(e)) return e
                case _ =>
            }
        }
        throw new RuntimeException()  // Für Intellij-Scala-Plugin 5.12.2011
    }

    def poll(t: Time): Event = tryPoll(t) getOrElse {throw new RuntimeException("Event has not arrived within "+t)}

    def tryPoll(t: Time) = Option(queue.poll(t.getMillis, TimeUnit.MILLISECONDS))
}

object EventPipe {
    private val lastEventClass = classOf[TerminatedEvent]
}

