package com.sos.scheduler.engine.main

import com.google.inject.Injector
import com.sos.scheduler.engine.eventbus.SchedulerEventBus
import java.time.Duration

/**
 * Steuerung für den C++-Scheduler in einem eigenen nebenläufigen Thread.
 * @author Joacim Zschimmer
 */
trait SchedulerController extends AutoCloseable {
  def startScheduler(arguments: String*): Unit

  /** Veranlasst die Beendigung des Schedulers, wartet aufs Ende und schließt alles. */
  def close(): Unit

  /** Veranlasst die Beendigung des Schedulers. */
  def terminateScheduler(): Unit

  /** Veranlasst die Beendigung des Schedulers nach einem Fehler.
   * Kann aus einem anderen Thread aufgerufen werden und lässt die Warte-Methoden die Exception werfen. */
  def terminateAfterException(x: Throwable): Unit

  def tryWaitForTermination(timeout: Duration): Boolean

  def exitCode: Int

  def eventBus: SchedulerEventBus

  def injector: Injector
}
