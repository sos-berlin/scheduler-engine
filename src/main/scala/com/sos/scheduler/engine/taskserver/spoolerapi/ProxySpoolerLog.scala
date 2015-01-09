package com.sos.scheduler.engine.taskserver.spoolerapi

import com.sos.scheduler.engine.common.scalautil.Logger
import com.sos.scheduler.engine.data.log.SchedulerLogLevel
import com.sos.scheduler.engine.minicom.comrpc.calls.ProxyId
import com.sos.scheduler.engine.minicom.comrpc.{ProxyIDispatchFactory, SpecializedProxyIDispatch, ProxyIDispatch, Remoting}
import com.sos.scheduler.engine.minicom.types.{CLSID, DISPATCH_METHOD, DISPID}
import com.sos.scheduler.engine.taskserver.spoolerapi.ProxySpoolerLog.LogDispId
import java.util.UUID

/**
 * @author Joacim Zschimmer
 */
final class ProxySpoolerLog(
  protected val remoting: Remoting,
  val id: ProxyId,
  val name: String)
extends SpoolerLog with SpecializedProxyIDispatch {

  def info(message: String): Unit = doLog(SchedulerLogLevel.info, message)

  private def doLog(level: SchedulerLogLevel, message: String) =
    invoke(LogDispId, DISPATCH_METHOD, Vector(level.cppNumber, message))
}

object ProxySpoolerLog extends ProxyIDispatchFactory {
  val clsid = CLSID(UUID.fromString("feee47a6-6c1b-11d8-8103-000476ee8afb"))
  private val LogDispId = DISPID(14)
  private val logger = Logger(getClass)

  def apply(remoting: Remoting, id: ProxyId, name: String, properties: Iterable[(String, Any)]) = {
    if (properties.nonEmpty) logger.warn(s"IGNORED: $properties")
    new ProxySpoolerLog(remoting, id, name)
  }
}
