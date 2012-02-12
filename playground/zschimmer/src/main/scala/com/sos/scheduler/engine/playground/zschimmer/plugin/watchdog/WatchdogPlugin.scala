package com.sos.scheduler.engine.playground.zschimmer.plugin.watchdog

import com.sos.scheduler.engine.kernel.Scheduler
import com.sos.scheduler.engine.kernel.plugin.Plugin
import com.sos.scheduler.engine.playground.zschimmer._
import com.sos.scheduler.engine.playground.zschimmer.Threads._
import org.apache.log4j.Logger
import javax.inject.Inject
import javax.annotation.Nullable
import org.w3c.dom.Element

/**Nicht für Produktion. Spielwiese für Scala-Rendezvous.
 * Prüft periodisch, ob der Scheduler reagiert, also ob er die große Scheduler-Schleife durchläuft und dabei einen API-Aufruf annimmt.
 * Das Plugin startet zwei Threads. Mit nur einem Thread kämen wir aus, wenn der Scheduler selbst periodisch eine Java-Methode aufriefe
 * (mit Async_operation) und das Plugin in eimem Thread prüft, ob die Aufrufe rechtzeitig erfolgen.
 *
 */
class WatchdogPlugin @Inject()(scheduler: Scheduler, @Nullable confElement: Element) extends Plugin {
  import WatchdogPlugin._

  require(confElement != null, "Missing configuration for "+classOf[WatchdogPlugin].getSimpleName)

  private val elem = XMLs.fromJavaDom(confElement).asInstanceOf[xml.Elem]
  private val conf = Configuration(elem)
  private val thread1 = new Thread1
  private val thread2 = new Thread2

  def activate() {
    thread1.start()
    thread2.start()
  }

  def close() {
    try interruptAndJoinThread(thread1)
    finally interruptAndJoinThread(thread2)
  }

  def getXmlState = "<watchdogPlugIn/>"

  private class Thread1 extends Thread {
    override def run() {
      untilInterruptedEvery(conf.checkEvery) {
        thread2.callImpatient((), conf.timeout, conf.warnEvery) {
          t => logger.warn("Scheduler does not respond after " + t)
        }
      }
    }
  }

  private class Thread2 extends Thread with Rendezvous[Unit, Unit] {
    override def run() {
      serveCalls {
        untilInterrupted {
          acceptCall { arg =>
              val t = new Timer(conf.timeout)
              scheduler.callCppAndDoNothing()
              if (t.isElapsed) logger.warn("Scheduler response time was " + t)
          }
        }
      }
    }
  }

}

object WatchdogPlugin {
  private val logger = Logger.getLogger(classOf[WatchdogPlugin])
}
