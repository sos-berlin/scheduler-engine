package com.sos.scheduler.engine.tests.jira.js1791

import com.sos.scheduler.engine.common.async.{CallQueue, TimedCall}
import com.sos.scheduler.engine.common.scalautil.Logger
import com.sos.scheduler.engine.data.event.KeyedEvent
import com.sos.scheduler.engine.data.events.custom.{CustomEvent, VariablesCustomEvent}
import com.sos.scheduler.engine.eventbus.EventPublisher
import com.sos.scheduler.engine.kernel.plugin.Plugin
import com.sos.scheduler.engine.tests.jira.js1791.TestPlugin._
import java.lang.Thread.currentThread
import java.util.concurrent.atomic.AtomicInteger
import javax.inject.Inject
import scala.util.control.NonFatal
import com.sos.scheduler.engine.common.time.ScalaTime._
import java.lang.System.currentTimeMillis

/**
  * @author Joacim Zschimmer
  */
final class TestPlugin @Inject private(eventPublisher: EventPublisher) extends Plugin
{
  override def onActivate() =
    for (_ ← 1 to Runtime.getRuntime.availableProcessors) {
      new TestThread().start()
    }

  private class TestThread extends Thread(getClass.getName) {
    override def run(): Unit = {
      sleep(100.ms)  // Time to start other threads, so all threads run concurrently
      val until = currentTimeMillis + duration.toMillis
      logger.info(s"Thread #${currentThread.getId} started")
      while (isActive && currentTimeMillis <= until) {
        count.incrementAndGet()
        val event = VariablesCustomEvent(Map("thread" → currentThread.getId.toString))
        try eventPublisher.publishCustomEvent(KeyedEvent(event)(CustomEvent.Key("KEY")))
        catch {
          case NonFatal(t) if isActive ⇒
            exception = Some(t)
          case _: CallQueue.ClosedException | _: TimedCall.CancelledException ⇒
            return
        }
        Thread.`yield`()
      }
      logger.info(s"Thread #${currentThread.getId} ended")
    }
  }
}

private[js1791] object TestPlugin {
  val duration = 3.s
  private val logger = Logger(getClass)
  val count = new AtomicInteger(0)
  @volatile
  var exception: Option[Throwable] = None
}
