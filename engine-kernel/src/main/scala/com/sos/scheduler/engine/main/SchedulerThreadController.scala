package com.sos.scheduler.engine.main

import com.sos.jobscheduler.common.scalautil.Logger
import com.sos.jobscheduler.common.time.ScalaTime._
import com.sos.jobscheduler.common.time.Stopwatch
import com.sos.scheduler.engine.common.sync.ThrowableMailbox
import com.sos.scheduler.engine.eventbus.SchedulerEventBus
import com.sos.scheduler.engine.kernel.Scheduler
import com.sos.scheduler.engine.kernel.settings.CppSettings
import com.sos.scheduler.engine.main.SchedulerThreadController._
import java.io.File
import java.time.Duration
import java.util.concurrent.TimeUnit
import org.scalactic.Requirements._
import scala.collection.JavaConversions._

/**
 * Steuert den [[SchedulerThread]].
 *
 * @author Joacim Zschimmer
 */
final class SchedulerThreadController(val name: String, cppSettings: CppSettings) extends SchedulerController {
  private val _eventBus = new SchedulerEventBus
  private val throwableMailbox: ThrowableMailbox[Throwable] = new ThrowableMailbox[Throwable]
  private val controllerBridge = new SchedulerThreadControllerBridge(this, eventBus, cppSettings)
  private val thread = new SchedulerThread(controllerBridge)
  private var _isStarted: Boolean = false

  def injector = controllerBridge.injector

  def loadModule(cppModuleFile: File): Unit =
    SchedulerThread.loadModule(cppModuleFile)

  def startScheduler(args: String*): Unit = {
    checkIsNotStarted()
    controllerBridge.start()
    thread.startThread(args)
    _isStarted = true
  }

  def close(): Unit = {
    val stopwatch = new Stopwatch
    terminateScheduler()
    if (!tryJoinThread(terminationTimeout)) {
      logger.warn(s"Still waiting for JobScheduler termination (${terminationTimeout.pretty}) ...")
      tryJoinThread(Duration.ofMillis(Long.MaxValue))
      logger.info(s"JobScheduler has been terminated after $stopwatch")
    }
    controllerBridge.close()
    eventBus.dispatchEvents()  // Thread-sicher, weil der Scheduler-Thread beendet ist, also selbst kein dispatchEvents() mehr aufrufen kann.
    throwableMailbox.throwUncheckedIfSet()
  }

  def waitUntilSchedulerState(s: BridgeState): Scheduler = {
    checkIsStarted()
    val scheduler = controllerBridge.waitUntilSchedulerState(s)
    throwableMailbox.throwUncheckedIfSet()
    scheduler
  }

  def terminateAfterException(t: Throwable): Unit = {
    throwableMailbox.setIfFirst(t)
    terminateScheduler()
  }

  private[main] def setThrowable(t: Throwable): Unit =
    throwableMailbox.setIfFirst(t)

  def terminateScheduler(): Unit = controllerBridge.terminate()

  def tryWaitForTermination(timeout: Duration): Boolean = {
    checkIsStarted()
    val result = tryJoinThread(timeout)
    throwableMailbox.throwUncheckedIfSet()
    result
  }

  private def tryJoinThread(timeout: Duration): Boolean = {
    if (timeout.toMillis == Long.MaxValue) {
      thread.join()
    } else {
      TimeUnit.MILLISECONDS.timedJoin(thread, timeout.toMillis)
    }
    !thread.isAlive
  }

  def checkIsNotStarted(): Unit = requireState(!isStarted, "Scheduler has already been started")

  private def checkIsStarted(): Unit = requireState(isStarted, "Scheduler has not been started")

  def isStarted = _isStarted

  def exitCode: Int = thread.exitCode

  def eventBus = _eventBus

  def getName = name
}

object SchedulerThreadController {
  private val logger = Logger(getClass)
  private val terminationTimeout = 5.s
}
