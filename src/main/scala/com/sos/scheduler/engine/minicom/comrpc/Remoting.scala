package com.sos.scheduler.engine.minicom.comrpc

import com.sos.scheduler.engine.minicom.comrpc.calls.{Call, ProxyId}
import com.sos.scheduler.engine.minicom.types.{CLSID, IDispatchable}

/**
 * @author Joacim Zschimmer
 */
trait Remoting {
  private[comrpc] def newProxy(proxyId: ProxyId, name: String, proxyClsid: CLSID, properties: Iterable[(String, Any)]): ProxyIDispatch

  private[comrpc] def iDispatchable(proxyId: ProxyId): IDispatchable

  private[comrpc] def sendReceive(call: Call): ResultDeserializer
}
