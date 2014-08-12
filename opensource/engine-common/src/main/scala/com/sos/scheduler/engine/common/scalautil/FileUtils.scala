package com.sos.scheduler.engine.common.scalautil

import com.google.common.base.Charsets.UTF_8
import com.google.common.io.Files
import java.io.File
import java.nio.charset.Charset

object FileUtils {
  object implicits {
    implicit class RichFile(val delegate: File) extends AnyVal {

      def /(filename: String) =
        new File(delegate, filename)

      def contentBytes: Array[Byte] =
        Files.toByteArray(delegate)

      def contentBytes_=(o: Array[Byte]) {
        Files.write(o, delegate)
      }

      def contentString: String = contentString(UTF_8)

      def contentString_=(o: String) {
        write(o, UTF_8)
      }

      def contentString(encoding: Charset) =
        Files.toString(delegate, encoding)

      def write(string: String, encoding: Charset = UTF_8) {
        Files.write(string, delegate, encoding)
      }

      def append(o: String, encoding: Charset = UTF_8) {
        Files.append(o, delegate, encoding)
      }
    }
  }
}
