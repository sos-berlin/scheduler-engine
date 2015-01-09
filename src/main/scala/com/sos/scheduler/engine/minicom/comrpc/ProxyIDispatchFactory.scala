package com.sos.scheduler.engine.minicom.comrpc

import com.sos.scheduler.engine.minicom.comrpc.calls.ProxyId
import com.sos.scheduler.engine.minicom.types.CLSID

/**
 * @author Joacim Zschimmer
 */
trait ProxyIDispatchFactory {
  val clsid: CLSID

  def apply(remoting: Remoting, id: ProxyId, name: String, proxyProperties: Iterable[(String, Any)]): ProxyIDispatch
}

object ProxyIDispatchFactory {
  type Fun = (Remoting, ProxyId, String, Iterable[(String, Any)]) ⇒ ProxyIDispatch
}
