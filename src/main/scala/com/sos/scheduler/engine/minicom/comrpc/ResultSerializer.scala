package com.sos.scheduler.engine.minicom.comrpc

import com.sos.scheduler.engine.minicom.comrpc.calls.{CreateInstanceResult, EmptyResult, InvokeResult, Result}
import com.sos.scheduler.engine.minicom.types.HRESULT.S_OK

/**
 * @author Joacim Zschimmer
 */
private final class ResultSerializer(protected val iunknownProxyRegister: ProxyRegister) extends IUnknownSerializer {

  def writeResult(result: Result): Unit = {
    writeByte(MessageClass.Answer)
    result match {

      case CreateInstanceResult(iUnknown) ⇒
        writeInt32(S_OK.value)
        writeInt32(S_OK.value)  // For IID
        writeIUnknown(Some(iUnknown))

      case InvokeResult(value) ⇒
        writeInt32(S_OK.value)
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
