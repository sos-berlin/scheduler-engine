package com.sos.scheduler.engine.kernel.log

import com.sos.jobscheduler.common.guice.GuiceImplicits.RichInjector
import com.sos.jobscheduler.common.scalautil.{Logger, ScalaConcurrentHashSet, SetOnce}
import com.sos.jobscheduler.data.log.{SchedulerLogLevel, SchedulerLogger}
import com.sos.scheduler.engine.cplusplus.runtime.annotation.ForCpp
import com.sos.scheduler.engine.cplusplus.runtime.{Sister, SisterType}
import com.sos.scheduler.engine.kernel.Scheduler
import com.sos.scheduler.engine.kernel.async.SchedulerThreadCallQueue
import com.sos.scheduler.engine.kernel.async.SchedulerThreadFutures.inSchedulerThread
import com.sos.scheduler.engine.kernel.cppproxy.Prefix_logC
import com.sos.scheduler.engine.kernel.scheduler.HasInjector
import java.io.File
import scala.util.control.NonFatal

/**
 * @author Joacim Zschimmer
 */
@ForCpp
final class PrefixLog(
  cppProxy: Prefix_logC,
  schedulerThreadCallQueueOption: Option[SchedulerThreadCallQueue])
extends Sister with SchedulerLogger {

  private val schedulerThreadCallQueueOnce = new SetOnce[SchedulerThreadCallQueue]

  schedulerThreadCallQueueOption foreach { schedulerThreadCallQueueOnce := _ }

  private implicit def schedulerThreadCallQueue = schedulerThreadCallQueueOnce getOrElse {
    throw new IllegalStateException("PrefixLog has not get a SchedulerThreadCallQueue") }

  private val subscriptions = new ScalaConcurrentHashSet[LogSubscription]()

  def onCppProxyInvalidated() = {}

  @ForCpp private def onStarted(scheduler: Scheduler): Unit = {
    if (schedulerThreadCallQueueOnce.isEmpty) {
      schedulerThreadCallQueueOnce.trySet(scheduler.injector.instance[SchedulerThreadCallQueue])
    }
    forAllSubscriptions { _.onStarted() }
  }

  @ForCpp private def onClosed(): Unit = {
    forAllSubscriptions { _.onClosed() }
    subscriptions.clear()
  }

  @ForCpp private def onLogged(): Unit = forAllSubscriptions { _.onLogged() }

  private def forAllSubscriptions(f: LogSubscription ⇒ Unit): Unit =
    for (o ← subscriptions) {
      try f(o)
      catch {
        case NonFatal(t) ⇒ PrefixLog.logger.error(s"$o: $t", t)
      }
    }

  def subscribe(o: LogSubscription): Unit = subscriptions += o

  def unsubscribe(o: LogSubscription): Unit = subscriptions -= o

  def log(level: SchedulerLogLevel, s: String): Unit = inSchedulerThread { cppProxy.java_log(level.cppNumber, s) }

  /** @return "", wenn für den Level keine Meldung vorliegt. */
  def lastByLevel(level: SchedulerLogLevel): String = inSchedulerThread { cppProxy.java_last(level.cppName) }

  def isStarted: Boolean = inSchedulerThread { cppProxy.started }

  def file: File = inSchedulerThread { new File(cppProxy.this_filename) }
}

object PrefixLog {
  private val logger = Logger(getClass)

  class Type extends SisterType[PrefixLog, Prefix_logC] {
    final def sister(proxy: Prefix_logC, context: Sister): PrefixLog = {
      val schedulerThreadCallQueueOption = Option(context) map { _.asInstanceOf[HasInjector].injector.instance[SchedulerThreadCallQueue] }
      new PrefixLog(proxy, schedulerThreadCallQueueOption)
    }
  }
}
