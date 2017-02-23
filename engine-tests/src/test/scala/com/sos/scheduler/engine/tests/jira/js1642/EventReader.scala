package com.sos.scheduler.engine.tests.jira.js1642

import com.google.common.io.Closer
import com.sos.jobscheduler.common.event.EventIdGenerator
import com.sos.jobscheduler.common.scalautil.Futures.implicits._
import com.sos.jobscheduler.common.scalautil.Logger
import com.sos.jobscheduler.common.time.ScalaTime._
import com.sos.jobscheduler.common.time.WaitForCondition.waitForCondition
import com.sos.jobscheduler.data.event.{AnyKeyedEvent, Event, EventId, EventRequest, EventSeq, KeyedEvent, Snapshot}
import com.sos.jobscheduler.data.filebased.TypedPath
import com.sos.scheduler.engine.client.web.WebSchedulerClient
import com.sos.scheduler.engine.data.events.SchedulerAnyKeyedEventJsonFormat
import com.sos.scheduler.engine.data.filebased.{FileBasedActivated, FileBasedEvent, UnknownTypedPath}
import com.sos.scheduler.engine.data.log.Logged
import com.sos.scheduler.engine.test.TestSchedulerController
import com.sos.scheduler.engine.tests.jira.js1642.EventReader._
import org.scalatest.Matchers._
import scala.collection.mutable
import scala.concurrent.ExecutionContext

/**
  * @author Joacim Zschimmer
  */
final class EventReader(webSchedulerClient: WebSchedulerClient, eventIdGenerator: EventIdGenerator, controller: TestSchedulerController)
  (implicit closer: Closer, ec: ExecutionContext)
extends AutoCloseable {

  private val eventBus = controller.eventBus
  private val directEvents = mutable.Buffer[AnyKeyedEvent]()
  private val webEvents = mutable.Buffer[AnyKeyedEvent]()
  private var stopping = false
  private var activatedEventId = EventId.BeforeFirst

  def start(): Unit = {
    activatedEventId = eventIdGenerator.lastUsedEventId  // Web events before Scheduler activation are ignored
    eventBus.onHot[Event] {
      case event if SchedulerAnyKeyedEventJsonFormat canSerialize event ⇒
        if (isPermitted(event)) {
          directEvents += event
        }
    }
    start(EventId.BeforeFirst)
  }

  def close(): Unit = {
    stopping = true
  }

  private def start(after: EventId): Unit = {
    (for (Snapshot(_, EventSeq.NonEmpty(eventSnapshots)) ← webSchedulerClient.events(EventRequest.singleClass[Event](after, 600.s)).appendCurrentStackTrace) yield {
      this.webEvents ++= eventSnapshots filter { snapshot ⇒ snapshot.eventId > activatedEventId && isPermitted(snapshot.value) } map { _.value }
      if (!stopping) {
        start(after = eventSnapshots.last.eventId)
      }
    })
    .failed foreach { throwable ⇒
      if (!stopping) {
        logger.error(s"webSchedulerClient.events: $throwable", throwable)
        controller.terminateAfterException(throwable)
      }
    }
  }

  private def isPermitted(event: AnyKeyedEvent) =
    event match {
      case KeyedEvent(_, FileBasedActivated) ⇒ this.webEvents.nonEmpty   // directEvents miss activation events at start
      case KeyedEvent(_, e: Logged) ⇒ false
      case _ ⇒ true
    }

  def check() = {
    assert(directEvents.nonEmpty)
    waitForCondition(5.s, 100.ms) { webEvents.size == directEvents.size }
    assert(webEvents == directEvents)
    val untypedPathDirectEvents = directEvents map {
      case KeyedEvent(key: TypedPath, e: FileBasedEvent) ⇒ KeyedEvent(e)(key.asTyped[UnknownTypedPath])  // Trait TypedPath is not properly deserializable
      case o ⇒ o
    }
    assert(webEvents == untypedPathDirectEvents)
  }
}

object EventReader {
  private val logger = Logger(getClass)
}
