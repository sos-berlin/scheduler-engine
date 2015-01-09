package com.sos.scheduler.engine.taskserver.spoolerapi

import com.sos.scheduler.engine.minicom.types.IDispatchable

/**
 * @author Joacim Zschimmer
 */
trait SpoolerLog extends IDispatchable {
  def info(message: String): Unit
}
