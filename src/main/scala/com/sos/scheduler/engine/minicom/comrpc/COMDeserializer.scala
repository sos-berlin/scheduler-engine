package com.sos.scheduler.engine.minicom.comrpc

import com.sos.scheduler.engine.common.scalautil.Logger
import com.sos.scheduler.engine.minicom.comrpc.COMDeserializer._
import com.sos.scheduler.engine.minicom.types.Variant._
import com.sos.scheduler.engine.minicom.types.{EmptyVariant, IUnknown, VariantArray}
import java.nio.ByteBuffer
import java.util.UUID
import org.scalactic.Requirements._

/**
 * @author Joacim Zschimmer
 */
private[comrpc] trait COMDeserializer {

  val buffer: ByteBuffer

  def readVariant(): Any = {
    val vt = readInt32()
    vt match {
      case _ if (vt & VT_ARRAY) != 0 ⇒ readVariantArray()
      case VT_UNKNOWN | VT_DISPATCH ⇒ readIUnknown()
      case _ ⇒ readSimpleVariant(vt)
    }
  }

  private def readVariantArray(): VariantArray = {
    val dimensions = readInt16()
    require(dimensions == 1)
    val features = readInt16()
    logger.debug(f"readVariantArray features=$features%x")
    //??? require(features == 0)
    val count = readInt32()
    val lowerBound = readInt32()
    require(lowerBound == 0)
    readInt32() match {
      case VT_UI1 ⇒ ???
      case VT_BSTR ⇒ ???
      case VT_VARIANT ⇒ VariantArray(Vector.fill(count) { readVariant() })
      case o ⇒ throw new IllegalArgumentException(f"Unsupported Array Variant VT=$o%x")
    }
  }

  private def readSimpleVariant(vt: Int): Any =
    vt match {
      case VT_EMPTY ⇒ EmptyVariant
      //case VT_NULL ⇒
      //case VT_I2 ⇒
      case VT_I4 | VT_INT ⇒ readInt32()
      //case VT_R4 ⇒
      //case VT_R8 ⇒
      //case VT_CY ⇒
      //case VT_DATE ⇒
      case VT_BSTR ⇒ readString()
      //case VT_ERROR ⇒
      case VT_BOOL ⇒ readBoolean()
      //case VT_DECIMAL ⇒
      //case VT_I1 ⇒
      //case VT_UI1 ⇒
      //case VT_UI2 ⇒
      //case VT_UI4 ⇒ IntVariant(readInt32())
      //case VT_I8 ⇒
      //case VT_UI8 ⇒
      //case VT_INT ⇒
      //case VT_UINT ⇒
      case o ⇒ throw new IllegalArgumentException(f"Unsupported Variant VT=$o%x")
    }

  def readInt16(): Int = buffer.getShort

  def readInt32(): Int = buffer.getInt

  def readInt64(): Long = buffer.getLong

  def readBoolean(): Boolean = buffer.get != 0

  def readByte(): Byte = buffer.get

  def readUUID(): UUID = {
    val high = buffer.getLong
    val low = buffer.getLong
    new UUID(high, low)
  }

  def readString(): String = {
    val length = buffer.getInt
    val b = new StringBuffer(length)
    for (i ← 1 to length) b.append(iso88591ByteToChar(buffer.get))
    b.toString
  }

  def readIUnknown(): IUnknown
}

private object COMDeserializer {
  private val logger = Logger(getClass)

  private def iso88591ByteToChar(o: Byte) = (o.toInt & 0xFF).toChar  // ISO-8859-1
}
