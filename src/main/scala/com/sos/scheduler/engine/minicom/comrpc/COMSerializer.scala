package com.sos.scheduler.engine.minicom.comrpc

import com.sos.scheduler.engine.minicom.comrpc.COMSerializer._
import com.sos.scheduler.engine.minicom.types.Variant._
import com.sos.scheduler.engine.minicom.types.{EmptyVariant, IDispatch, IUnknown}
import java.nio.ByteBuffer
import java.util.UUID

/**
 * @author Joacim Zschimmer
 */
private[comrpc] trait COMSerializer {

  val byteBuffer = ByteBuffer.allocate(10000)   // TODO Puffer soll wachsen können -> Mehrere ByteBuffer verketten

  final def writeVariant(value: Any): Unit = {
    value match {
      case o: Int ⇒ writeInt32(VT_I4); writeInt32(o)
      case o: Long ⇒ writeInt32(VT_I8); writeInt64(o)
      case o: Boolean ⇒ writeInt32(VT_BOOL); writeBoolean(o)
      case o: String ⇒ writeInt32(VT_BSTR); writeString(o)
      case o: IDispatch ⇒ writeInt32(VT_DISPATCH); writeIUnknown(o)
      case o: IUnknown ⇒ writeInt32(VT_UNKNOWN); writeIUnknown(o)
      case EmptyVariant ⇒ writeInt32(VT_EMPTY)
    }
  }

  final def writeInt32(o: Int): Unit = byteBuffer.putInt(o)

  final def writeInt64(o: Long): Unit = byteBuffer.putLong(o)

  final def writeBoolean(o: Boolean): Unit = byteBuffer.put(if (o) 1.toByte else 0.toByte)

  final def writeByte(o: Byte): Unit = byteBuffer.put(o)

  final def writeUUID(o: UUID): Unit = byteBuffer.putLong(o.getMostSignificantBits).putLong(o.getLeastSignificantBits)

  final def writeString(o: String): Unit = {
    byteBuffer.putInt(o.length)
    for (c ← o.iterator) byteBuffer.put(charToByte(c))
  }

  def writeIUnknown(iUnknown: IUnknown): Unit
}

private object COMSerializer {
  private def charToByte(o: Char) = o.toByte    // TODO Codierung?
}
