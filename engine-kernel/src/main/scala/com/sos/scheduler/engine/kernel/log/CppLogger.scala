package com.sos.scheduler.engine.kernel.log

import com.sos.scheduler.engine.data.log.SchedulerLogLevel
import org.slf4j.{Logger, LoggerFactory}
import com.google.common.base.Splitter
import scala.collection.JavaConversions._

object CppLogger {

  private lazy val logger = LoggerFactory.getLogger("JobScheduler")

  def log(prefix: String, level: SchedulerLogLevel, lines: String): Unit = {
    if (level != SchedulerLogLevel.none) {
      for (line <- splitLines(lines)) {
        val prefixedLine = if (prefix.isEmpty) line else s"($prefix) $line"
        logLine(logger, level, prefixedLine)
      }
    }
  }

  private def logLine(logger: Logger, level: SchedulerLogLevel, line: String): Unit = {
    level match {
      case SchedulerLogLevel.error   => logger.error(line)
      case SchedulerLogLevel.warning => logger.warn(line)
      case SchedulerLogLevel.info    => logger.info(line)
      case SchedulerLogLevel.debug9  => logger.trace(line)
      case _ =>
        if (level.cppNumber <= SchedulerLogLevel.debug3.cppNumber) logger.trace(line)
        else logger.debug(line)
    }
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
