package com.sos.scheduler.engine.minicom.comrpc

import com.sos.scheduler.engine.minicom.comrpc.calls.{CreateInstanceResult, EmptyResult, InvokeResult, Result}
import com.sos.scheduler.engine.minicom.types.hresult

/**
 * @author Joacim Zschimmer
 */
private final class ResultSerializer(protected val iunknownProxyRegister: ProxyRegister) extends IUnknownSerializer {

  def writeResult(result: Result): Unit = {
    writeByte(MessageClass.Answer)
    result match {

      case CreateInstanceResult(iUnknown) ⇒
        writeInt32(hresult.S_OK)
        writeInt32(hresult.S_OK)  // For IID
        writeIUnknown(Some(iUnknown))

      case InvokeResult(value) ⇒
        writeInt32(hresult.S_OK)
        writeVariant(value)

      case EmptyResult ⇒
    }
  }
}

object ResultSerializer {
  /**
   * @return (Array, length)
   */
  def serializeResult(proxyRegister: ProxyRegister, result: Result): (Array[Byte], Int) = {
    val serializer = new ResultSerializer(proxyRegister)
    serializer.writeResult(result)
    serializer.byteArrayAndLength
  }
}
