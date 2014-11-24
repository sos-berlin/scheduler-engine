package com.sos.scheduler.engine.test.scalatest

import org.slf4j.LoggerFactory

object Utils {
  private val logger = LoggerFactory.getLogger(getClass)

  def ignoreException[A](f: => A) =
    try f
    catch { case x: Throwable => logger.error("Ignored", x) }
}
