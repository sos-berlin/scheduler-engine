package com.sos.scheduler.engine.tests.jira.js1791

import com.sos.scheduler.engine.common.scalautil.{Logger, ScalaConcurrentHashSet}
import com.sos.scheduler.engine.common.time.ScalaTime._
import com.sos.scheduler.engine.data.events.custom.VariablesCustomEvent
import com.sos.scheduler.engine.test.scalatest.ScalaSchedulerTest
import com.sos.scheduler.engine.tests.jira.js1791.JS1791IT._
import java.lang.Thread.currentThread
import java.util.concurrent.atomic.{AtomicInteger, AtomicReference}
import org.junit.runner.RunWith
import org.scalatest.FreeSpec
import org.scalatest.junit.JUnitRunner

/**
  * @author Joacim Zschimmer
  */
@RunWith(classOf[JUnitRunner])
final class JS1791IT extends FreeSpec with ScalaSchedulerTest
{
  private val firstThread = new AtomicReference[Thread](null)
  private val threads = new ScalaConcurrentHashSet[String]
  private val eventCount = new AtomicInteger(0)
  @volatile
  private var mixedThreads = false

  eventBus.onHot[VariablesCustomEvent] { case keyedEvent ⇒
    threads += keyedEvent.event.variables("thread")
    firstThread.compareAndSet(null, currentThread)
    if (currentThread ne firstThread.get) {
      mixedThreads = true
    }
    eventCount.incrementAndGet()
  }

  "EventPublisher.publishJava is thread-safe" in {
    // Check manually for HotEventBus message similar to "ignoring this event while handling the event" !!!
    sleep(TestPlugin.duration)
    scheduler executeXml <terminate/>
    logger.info(s"TestPlugin.count.get=${TestPlugin.count.get} eventCount=${eventCount.get}")
    assert(TestPlugin.exception == None)
    assert(TestPlugin.count.get > 1000)  // Should be about one million events
    assert(eventCount.get > 1000)
    assert(!mixedThreads)
    assert(threads.size == Runtime.getRuntime.availableProcessors)
  }
}

object JS1791IT {
  private val logger = Logger(getClass)
}
