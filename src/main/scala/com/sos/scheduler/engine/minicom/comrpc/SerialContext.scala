package com.sos.scheduler.engine.minicom.comrpc

import com.sos.scheduler.engine.minicom.comrpc.calls.{ProxyId, Call}
import com.sos.scheduler.engine.minicom.types.IDispatchable

/**
 * @author Joacim Zschimmer
 */
trait SerialContext {
  private[comrpc] def registerProxy(proxy: ProxyIDispatch): Unit

  private[comrpc] def iDispatchableOption(proxyId: ProxyId): Option[IDispatchable]

  private[comrpc] def sendReceive(call: Call): ResultDeserializer
}
