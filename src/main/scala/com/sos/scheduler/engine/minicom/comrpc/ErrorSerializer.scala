package com.sos.scheduler.engine.minicom.comrpc

import com.sos.scheduler.engine.minicom.types.COMException
import com.sos.scheduler.engine.minicom.types.HRESULT.DISP_E_EXCEPTION

/**
 * @author Joacim Zschimmer
 */
object ErrorSerializer {
  def serializeError(t: Throwable): (Array[Byte], Int) = {
    val b = new BaseSerializer
    b.writeByte(MessageClass.Error)
    b.writeString("name=COM")
    b.writeString(f"code=${throwableToHResult(t).comString}")
    b.writeString(s"what=$t")
    b.byteArrayAndLength
  }

  private def throwableToHResult(t: Throwable) = t match {
    case t: COMException ⇒ t.hResult
    case _ ⇒ DISP_E_EXCEPTION
  }
}
