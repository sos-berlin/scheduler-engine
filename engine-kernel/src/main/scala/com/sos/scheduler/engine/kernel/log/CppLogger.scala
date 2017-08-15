package com.sos.scheduler.engine.kernel.log

import com.google.common.base.Splitter
import com.sos.scheduler.engine.common.log.LogLevel
import com.sos.scheduler.engine.common.log.LogLevel.LevelLogger
import com.sos.scheduler.engine.data.log.SchedulerLogLevel
import org.slf4j.LoggerFactory
import scala.collection.JavaConversions._

object CppLogger {
  private lazy val logger = LoggerFactory.getLogger("JobScheduler")
  private val schedulerToLogLevel = Array(
    LogLevel.LogNone,   // none
    LogLevel.Trace,  // debug9
    LogLevel.Trace,  // debug8
    LogLevel.Trace,  // debug7
    LogLevel.Trace,  // debug6
    LogLevel.Trace,  // debug5
    LogLevel.Trace,  // debug4
    LogLevel.Debug,  // debug3
    LogLevel.Debug,  // debug2
    LogLevel.Debug,  // debug1
    LogLevel.Info,   // info
    LogLevel.Warn,   // warning
    LogLevel.Error)  // error

  def log(level: SchedulerLogLevel, prefix: String, lines: String): Unit = {
    val logLevel = toLogLevel(level)
    if (logger.isEnabled(logLevel)) {
      for (line ← splitLines(lines)) {
        val prefixedLine = if (prefix.isEmpty) line else s"($prefix) $line"
        logger.log(logLevel, prefixedLine)
      }
    }
  }

  private def toLogLevel(level: SchedulerLogLevel): LogLevel = {
    schedulerToLogLevel(level.cppNumber - SchedulerLogLevel.none.cppNumber)
  }

  private object splitLines {
    private val splitter = Splitter on '\n'

    def apply(lines: String): Iterable[String] = {
      if (lines contains '\n') {
        val splitted = (splitter split lines).view map rightTrim dropWhile { _.isEmpty }
        if (splitted.isEmpty) Iterable("") else splitted
      } else
        Iterable(rightTrim(lines))
    }
  }

  private def rightTrim(s: String) = {
    var i = s.length - 1
    if (i >= 0 && s.charAt(i) == '\r') i -= 1
    while (i >= 0 && s.charAt(i) == ' ') i -= 1
    s.substring(0, i + 1)
  }
}
