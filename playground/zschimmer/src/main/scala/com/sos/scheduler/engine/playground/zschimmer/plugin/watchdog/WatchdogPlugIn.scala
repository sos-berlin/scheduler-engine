package com.sos.scheduler.engine.playground.zschimmer.plugin.watchdog

import com.sos.scheduler.engine.kernel.Scheduler
import com.sos.scheduler.engine.kernel.plugin.PlugIn
import com.sos.scheduler.engine.playground.zschimmer._
import com.sos.scheduler.engine.playground.zschimmer.Threads._
import java.lang.Math.max
import org.apache.log4j.Logger

/** Nicht für Produktion. Spielwiese für Scala-Rendezvous.
 * Prüft periodisch, ob der Scheduler reagiert, also ob er die große Scheduler-Schleife durchläuft und dabei einen API-Aufruf annimmt.
 * Das PlugIn startet zwei Threads. Mit nur einem Thread kämen wir aus, wenn der Scheduler selbst periodisch eine Java-Methode aufriefe
 * (mit Async_operation) und das PlugIn in eimem Thread prüft, ob die Aufrufe rechtzeitig erfolgen.
 *
 */
class WatchdogPlugIn(scheduler: Scheduler, confElemOption: Option[xml.Elem]) extends PlugIn {
    import WatchdogPlugIn._
    private val thread1 = new Thread1
    private val thread2 = new Thread2
    private val conf = Configuration(confElemOption)


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
            untilInterrupted {
                val t = new Timer(conf.timeout)
                thread2.callImpatient((), conf.timeout, conf.warnEvery) { logger.warn("Scheduler does not respond after " + t) }
                Thread.sleep(max(0, conf.checkEvery.getMillis - t.elapsedMs))
            }
        }
    }

    private class Thread2 extends Thread with Rendezvous[Unit,Unit] {
        override def run() {
            serveCalls {
                untilInterrupted {
                    acceptCall { arg: Unit =>
                        val t = new Timer(conf.timeout)
                        scheduler.callCppAndDoNothing()
                        if (t.isElapsed)  logger.warn("Scheduler response time was " + t)
                    }
                }
            }
        }
    }
}


object WatchdogPlugIn {
    private val logger = Logger.getLogger(classOf[WatchdogPlugIn])

    def factory = new PlugInFactory {
        def newInstance(scheduler: Scheduler, conf: Option[xml.Elem]) = new WatchdogPlugIn(scheduler, conf)
    }
}
