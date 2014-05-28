package com.sos.scheduler.engine.playground.zschimmer.plugin.watchdog

import WatchdogPlugin._
import com.sos.scheduler.engine.common.scalautil.Logger
import com.sos.scheduler.engine.kernel.Scheduler
import com.sos.scheduler.engine.kernel.async.SchedulerThreadCallQueue
import com.sos.scheduler.engine.kernel.async.SchedulerThreadFutures._
import com.sos.scheduler.engine.kernel.plugin.{Plugin, Plugins}
import com.sos.scheduler.engine.playground.zschimmer.Threads._
import com.sos.scheduler.engine.playground.zschimmer.{XMLs, Timer}
import java.util.concurrent.TimeUnit
import javax.inject.{Named, Inject}
import org.w3c.dom.Element
import scala.concurrent._
import scala.concurrent.duration.Duration

/** Prüft periodisch, ob der Scheduler reagiert, also ob er die große Scheduler-Schleife durchläuft und dabei einen API-Aufruf annimmt.
  * Das Plugin startet zwei Threads. Mit nur einem Thread kämen wir aus, wenn der Scheduler selbst periodisch eine Java-Methode aufriefe
  * (mit Async_operation) und das Plugin in eimem Thread prüft, ob die Aufrufe rechtzeitig erfolgen.
  */
final class WatchdogPlugin @Inject private(
  scheduler: Scheduler,
  private implicit val schedulerThreadCallQueue: SchedulerThreadCallQueue,
  @Named(Plugins.configurationXMLName) confElement: Element)
extends Plugin {

  private val elem = XMLs.fromJavaDom(confElement).asInstanceOf[xml.Elem]
  private val configuration = Configuration(elem)
  private val thread1 = new Thread1

  override def onActivate() {
    thread1.start()
  }

  override def close() {
    interruptAndJoinThread(thread1)
  }

  override def xmlState =
    "<watchdogPlugin/>"

  private class Thread1 extends Thread {
    override def run() {
      untilInterruptedEvery(configuration.checkEvery) {
        val future = schedulerThreadFuture { scheduler.callCppAndDoNothing() }
        val t = new Timer(configuration.timeout)
        try {
          Await.result(future, Duration(configuration.timeout.getMillis, TimeUnit.MILLISECONDS))
          logger.info(s"Scheduler response time was $t")
        }
        catch {
          case _: TimeoutException => logger.warn(s"Scheduler does not respond after ${configuration.timeout.getStandardSeconds}s")
        }
      }
    }
  }
}

object WatchdogPlugin {
  private def logger = Logger(getClass)
}
