package com.sos.scheduler.engine.taskserver.spoolerapi

/**
 * @author Joacim Zschimmer
 */
trait SpoolerLog {
  def info(message: String): Unit
}
