package com.sos.scheduler.engine.plugins.newwebservice.routes.cpp

import java.nio.charset.{Charset, CharsetDecoder, CharsetEncoder, CoderResult}
import java.nio.{ByteBuffer, CharBuffer}

/**
  * Byte representation is simply the code of the UNICODE character modulo 256 (the lower byte).
  * This is as the C++ JobScheduler code handles characters: like bytes.
  *
  * @author Joacim Zschimmer
  */
object ByteCharset extends Charset("ByteCharset", Array()) {

  def contains(cs: Charset) = cs.name == name

  def newDecoder() = new CharsetDecoder(this, 1.0f, 1.0f) {
    def decodeLoop(in: ByteBuffer, out: CharBuffer) = {
      var remaining = Math.min(in.remaining, out.capacity - out.position)
      while (remaining > 0) {
        out.put((in.get().toInt & 0xff).toChar)
        remaining -= 1
      }
      if (in.remaining == 0) CoderResult.UNDERFLOW else CoderResult.OVERFLOW
    }
  }

  def newEncoder() = new CharsetEncoder(this, 1.0f, 1.0f) {
    def encodeLoop(in: CharBuffer, out: ByteBuffer) = {
      var remaining = Math.min(in.remaining, out.capacity - out.position)
      while (remaining > 0) {
        val char = in.get()
        out.put(char.toByte)
        remaining -= 1
      }
      if (in.remaining == 0) CoderResult.UNDERFLOW else CoderResult.OVERFLOW
    }
  }
}
