package com.sos.scheduler.engine.minicom.comrpc

import com.sos.scheduler.engine.common.scalautil.Logger
import com.sos.scheduler.engine.minicom.comrpc.calls.ProxyId
import com.sos.scheduler.engine.minicom.types.CLSID

/**
 * @author Joacim Zschimmer
 */
final class SimpleProxyIDispatch(
  protected val remoting: Remoting,
  val id: ProxyId,
  val name: String)
extends ProxyIDispatch

object SimpleProxyIDispatch extends ProxyIDispatchFactory {
  val clsid = CLSID.Null
  private val logger = Logger(getClass)

  def apply(remoting: Remoting, id: ProxyId, name: String, properties: Iterable[(String, Any)]) = {
    if (properties.nonEmpty) logger.warn(s"IGNORED: $properties")
    new SimpleProxyIDispatch(remoting, id, name)
  }
}
