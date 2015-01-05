package com.sos.scheduler.engine.minicom.comrpc

import com.sos.scheduler.engine.minicom.comrpc.BaseSerializer._
import java.nio.ByteBuffer
import java.util.UUID
import scala.math.max

/**
 * @author Joacim Zschimmer
 */
private[comrpc] class BaseSerializer {
  private var byteBuffer = ByteBuffer allocate InitialSize

  final def writeByte(o: Byte): Unit = {
    need(1)
    byteBuffer.put(o)
  }

  final def writeInt32(o: Int): Unit = {
    need(4)
    byteBuffer.putInt(o)
  }

  final def writeInt64(o: Long): Unit = {
    need(8)
    byteBuffer.putLong(o)
  }

  final def writeBoolean(o: Boolean): Unit = {
    need(1)
    byteBuffer.put(if (o) 1.toByte else 0.toByte)
  }

  final def writeString(o: String): Unit = {
    need(4 + o.length)
    byteBuffer.putInt(o.length)
    for (c ← o.iterator) byteBuffer.put(charToByte(c))
  }

  final def writeUUID(o: UUID): Unit = byteBuffer.putLong(o.getMostSignificantBits).putLong(o.getLeastSignificantBits)

  final def need(n: Int): Unit = {
    val neededSize = byteBuffer.position + n
    if (neededSize > byteBuffer.limit) {
      byteBuffer = ByteBuffer allocate increased(byteBuffer.limit, neededSize)
    }
  }

  def byteArrayAndLength = (byteBuffer.array, byteBuffer.position)
}

private object BaseSerializer {
  private[comrpc] val InitialSize = 1000
  private def charToByte(o: Char) = o.toByte    // TODO Codierung?
  
  private[comrpc] def increased(currentSize: Int, neededSize: Int) =
    max(2 * currentSize, neededSize + InitialSize)
}
