package com.sos.scheduler.engine.kernel.event.collector

import com.sos.scheduler.engine.data.event._
import scala.concurrent.{Future, Promise}

/**
  * @author Joacim Zschimmer
  */
private[collector] final class Sync {

  @volatile private var eventArrivedPromise = Promise[Unit]()
  @volatile private var promiseUsed = false
  @volatile private var lastEventId = EventId.BeforeFirst

  /** Not to be called concurrently. */
  def onNewEvent(eventId: EventId): Unit =
    synchronized {
      lastEventId = eventId
      if (promiseUsed) {
        eventArrivedPromise.trySuccess(())
        promiseUsed = false
        eventArrivedPromise = Promise[Unit]()
      }
    }

  def whenEventIsAvailable(after: EventId): Future[Unit] =
    if (after < lastEventId)
      Future.successful(())
    else
      synchronized {
        if (after < lastEventId)
          Future.successful(())
        else {
          promiseUsed = true
          eventArrivedPromise.future
        }
      }
}
