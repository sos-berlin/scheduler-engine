package com.sos.scheduler.engine.taskserver.spoolerapi

import com.sos.scheduler.engine.data.log.SchedulerLogLevel
import com.sos.scheduler.engine.minicom.comrpc.calls.ProxyId
import com.sos.scheduler.engine.minicom.comrpc.{MessageConnection, ProxyIDispatch, ProxyRegister}
import com.sos.scheduler.engine.minicom.types.{DISPATCH_METHOD, DISPID}
import com.sos.scheduler.engine.taskserver.spoolerapi.ProxySpoolerLog._

/**
 * @author Joacim Zschimmer
 */
final class ProxySpoolerLog(
  protected val connection: MessageConnection,
  protected val proxyRegister: ProxyRegister,
  val id: ProxyId,
  val name: String)
extends SpoolerLog with ProxyIDispatch {

  def info(message: String): Unit = doLog(SchedulerLogLevel.info, message)

  private def doLog(level: SchedulerLogLevel, message: String) =
    invoke(LogDispId, DISPATCH_METHOD, Vector(level.cppNumber, message))
}

private object ProxySpoolerLog {
  private val LogDispId = DISPID(14)
}
