package com.sos.scheduler.engine.kernel.event.collector

import com.sos.scheduler.engine.data.event._
import scala.concurrent.{Future, Promise}

/**
  * @author Joacim Zschimmer
  */
private[collector] abstract class Sync {

  protected def hasAfter(eventId: EventId): Boolean

  @volatile
  private var eventArrivedPromise = Promise[Unit]()
  @volatile
  private var used = false

  /** Not to be called concurrently. */
  final def onNewEvent(): Unit =
    synchronized {
      val p = eventArrivedPromise
      if (used) {
        used = false
        eventArrivedPromise = Promise[Unit]()
      }
      p.trySuccess(())
    }

  final def whenEventIsAvailable(after: EventId): Future[Unit] =
    if (hasAfter(after))
      Future.successful(())
    else
      synchronized {
        if (hasAfter(after))
          Future.successful(())
        else {
          used = true
          eventArrivedPromise.future
        }
      }
}
