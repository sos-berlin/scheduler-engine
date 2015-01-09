package com.sos.scheduler.engine.taskserver.spoolerapi

import com.sos.scheduler.engine.data.log.SchedulerLogLevel
import com.sos.scheduler.engine.minicom.comrpc.calls.ProxyId
import com.sos.scheduler.engine.minicom.comrpc.{ProxyIDispatch, SerialContext}
import com.sos.scheduler.engine.minicom.types.{CLSID, DISPATCH_METHOD, DISPID}
import com.sos.scheduler.engine.taskserver.spoolerapi.ProxySpoolerLog._
import java.util.UUID

/**
 * @author Joacim Zschimmer
 */
final class ProxySpoolerLog(
  protected val serialContext: SerialContext,
  val id: ProxyId,
  val name: String)
extends SpoolerLog with ProxyIDispatch {

  def info(message: String): Unit = doLog(SchedulerLogLevel.info, message)

  private def doLog(level: SchedulerLogLevel, message: String) =
    invoke(LogDispId, DISPATCH_METHOD, Vector(level.cppNumber, message))
}

object ProxySpoolerLog /*extends IDispatchFactory???*/ {
  val clsid = CLSID(UUID.fromString("feee47a6-6c1b-11d8-8103-000476ee8afb"))
  //val iid = IID.Null
  //def apply() = new ProxySpoolerLog
  private val LogDispId = DISPID(14)
}
