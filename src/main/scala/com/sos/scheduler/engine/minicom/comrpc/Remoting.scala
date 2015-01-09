package com.sos.scheduler.engine.minicom.comrpc

import com.sos.scheduler.engine.minicom.comrpc.calls.{ProxyId, Call}
import com.sos.scheduler.engine.minicom.types.IDispatchable

/**
 * @author Joacim Zschimmer
 */
trait Remoting {
  private[comrpc] def registerProxy(proxy: ProxyIDispatch): Unit

  private[comrpc] def iDispatchable(proxyId: ProxyId): IDispatchable

  private[comrpc] def sendReceive(call: Call): ResultDeserializer
}
