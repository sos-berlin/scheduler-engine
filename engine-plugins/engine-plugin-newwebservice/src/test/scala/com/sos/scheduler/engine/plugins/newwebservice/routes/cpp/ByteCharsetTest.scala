package com.sos.scheduler.engine.plugins.newwebservice.routes.cpp

import java.nio.ByteBuffer
import org.junit.runner.RunWith
import org.scalatest.FreeSpec
import org.scalatest.junit.JUnitRunner

/**
  * @author Joacim Zschimmer
  */
@RunWith(classOf[JUnitRunner])
final class ByteCharsetTest extends FreeSpec {

  "ByteCharset" in {
    val string = "abc123äöü"
    val bytes: Array[Byte] = Array(0x61, 0x62, 0x63, 0x31, 0x32, 0x33, 0xe4, 0xf6, 0xfc) map { _.toByte }
    assert(ByteCharset.decode(ByteBuffer.wrap(bytes)).toString == string)
    assert(java.util.Arrays.equals(ByteCharset.encode(string).array, bytes))
  }

  "Invalid character" in {
    assert('→'.toByte.toChar != '→')
    assert(ByteCharset.encode("→").get == '→'.toByte)
  }
}
