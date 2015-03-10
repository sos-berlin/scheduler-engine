package com.sos.scheduler.engine.kernel.log

import com.sos.scheduler.engine.common.scalautil.Logger
import com.sos.scheduler.engine.cplusplus.runtime.annotation.ForCpp
import com.sos.scheduler.engine.cplusplus.runtime.{Sister, SisterType}
import com.sos.scheduler.engine.data.log.{SchedulerLogLevel, SchedulerLogger}
import com.sos.scheduler.engine.kernel.cppproxy.Prefix_logC
import java.io.File
import scala.util.control.NonFatal

/**
 * @author Joacim Zschimmer
 */
@ForCpp
final class PrefixLog(cppProxy: Prefix_logC) extends Sister with SchedulerLogger {

  @volatile private var subscriptions = Set[LogSubscription]()

  def onCppProxyInvalidated() = {}

  @ForCpp private def onStarted(): Unit = forAllSubscriptions { _.onStarted() }

  @ForCpp private def onClosed(): Unit = {
    forAllSubscriptions { _.onClosed() }
    subscriptions = Set()
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

  def log(level: SchedulerLogLevel, s: String): Unit = cppProxy.java_log(level.cppNumber, s)

  /** @return "", wenn für den Level keine Meldung vorliegt. */
  def lastByLevel(level: SchedulerLogLevel): String = cppProxy.java_last(level.cppName)

  def isStarted: Boolean = cppProxy.started

  def file: File = new File(cppProxy.this_filename)
}

object PrefixLog {
  private val logger = Logger(getClass)

  class Type extends SisterType[PrefixLog, Prefix_logC] {
    final def sister(proxy: Prefix_logC, context: Sister): PrefixLog = {
      new PrefixLog(proxy)
    }
  }
}
