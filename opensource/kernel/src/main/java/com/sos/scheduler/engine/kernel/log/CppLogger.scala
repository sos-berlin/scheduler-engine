package com.sos.scheduler.engine.kernel.log

import com.sos.scheduler.engine.data.log.SchedulerLogLevel
import org.slf4j.{Logger, LoggerFactory}

object CppLogger {
  def log(prefix: String, level: SchedulerLogLevel, line: String) {
    if (level != SchedulerLogLevel.none) {
      val logger = LoggerFactory.getLogger("JobScheduler")
      val prefixedLine = if (prefix.isEmpty) line else s"($prefix) $line"
      log(logger, level, prefixedLine)
    }
  }

  def log(logger: Logger, level: SchedulerLogLevel, line: String) {
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
}
