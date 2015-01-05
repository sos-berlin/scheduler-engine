package com.sos.scheduler.engine.minicom.comrpc

import com.sos.scheduler.engine.minicom.types.hresult.DISP_E_EXCEPTION

/**
 * @author Joacim Zschimmer
 */
object ErrorSerializer {
  def serializeError(t: Throwable): (Array[Byte], Int) = {
    val b = new BaseSerializer
    b.writeByte(MessageClass.Error)
    b.writeString("name=COM")
    val hResult = DISP_E_EXCEPTION
    b.writeString(f"code=COM-$hResult%08X")
    b.writeString(s"what=$t")
    b.byteArrayAndLength
  }
}
