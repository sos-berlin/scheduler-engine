package com.sos.scheduler.taskserver.comrpc

import com.sos.scheduler.taskserver.comrpc.calls.{CallResult, CreateInstanceResult, EmptyResult, Result}
import com.sos.scheduler.taskserver.comrpc.types.hresult

/**
 * @author Joacim Zschimmer
 */
private[comrpc] final class CallSerializer(protected val iunknownProxyRegister: ProxyRegister) extends ProxySerializer {

  def writeResult(result: Result): Unit = {
    writeByte(MessageClass.Answer)
    result match {
      case CreateInstanceResult(iUnknown) ⇒
        writeInt32(hresult.S_OK)
        writeInt32(hresult.S_OK)  // For IID
        writeIUnknown(iUnknown)

      case CallResult(value) ⇒
        writeInt32(hresult.S_OK)
        writeVariant(value)

      case EmptyResult ⇒
    }
  }
}
