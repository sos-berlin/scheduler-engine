package com.sos.scheduler.engine.playground.zschimmer.plugin.watchdog

import com.sos.scheduler.engine.kernel.Scheduler
import com.sos.scheduler.engine.kernel.plugin.PlugIn
import com.sos.scheduler.engine.kernel.plugin.PlugInFactory
import com.sos.scheduler.engine.kernel.util.Time
import com.sos.scheduler.engine.playground.zschimmer.Rendezvous
import org.apache.log4j.Logger
import org.w3c.dom.Element

/** Nicht für Produktion. Spielwiese für Scala-Rendezvous.
 * Prüft periodisch, ob der Scheduler reagiert, also ob er die große Scheduler-Schleife durchläuft und dabei einen API-Aufruf annimmt.
 * Das PlugIn startet zwei Threads. Mit nur einem Thread kämen wir aus, wenn der Scheduler selbst periodisch eine Java-Methode aufriefe
 * (mit Async_operation) und das PlugIn in eimem Thread prüft, ob die Aufrufe rechtzeitig erfolgen.
 *
 */
class WatchdogPlugIn(scheduler: Scheduler, configuration: Option[xml.Elem]) extends PlugIn {
    import WatchdogPlugIn._
    private val thread1 = new Thread1
    private val thread2 = new Thread2
    private val periodMs = 5*100
    private val timeout = Time.ofMs(1*100)

    def activate() {
        thread1.start()
        thread2.start()
    }

    def close() {
        try thread1.close()
        finally thread2.close()
    }

    def getXmlState = "<watchdogPlugIn/>"
    
    private class Thread1 extends Thread with Terminatable {
        override def run() {
            untilTerminated {
                val timer = new Timer(timeout)
                thread2.callWithTimeout(true, timeout) { 
                    logger.warn("Scheduler does not respond after " + timer)
                    Time.of(1)
                }
                sleepThread(periodMs)
            }
        }
    }

    private class Thread2 extends Thread with Terminatable with Rendezvous[Boolean,Boolean] {
        override def run() {
            serve {
                untilTerminated {
                    acceptCall { arg: Boolean =>
                        val timer = new Timer(timeout)
                        scheduler.callCppAndDoNothing()
                        if (timer.isElapsed)  logger.warn("Scheduler response time was " + timer)
                        true
                    }
                }
            }
        }
    }
}


object WatchdogPlugIn {
    private val logger = Logger.getLogger(classOf[WatchdogPlugIn])

    def factory = new PlugInFactory {
        def newInstance(scheduler: Scheduler, configurationOrNull: Element) = {
            val elem = Option(configurationOrNull) map { XMLs.fromJavaDom(_).asInstanceOf[xml.Elem] }
            new WatchdogPlugIn(scheduler, elem)
        }
    }
}
