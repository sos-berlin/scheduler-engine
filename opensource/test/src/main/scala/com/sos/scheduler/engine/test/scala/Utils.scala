package com.sos.scheduler.engine.test.scala

import org.apache.log4j.Logger

object Utils {
  private val logger: Logger = Logger.getLogger(getClass)

  def ignoreException[A](f: => A) = try f catch { case x: Throwable => logger.error(x, x) }
}
