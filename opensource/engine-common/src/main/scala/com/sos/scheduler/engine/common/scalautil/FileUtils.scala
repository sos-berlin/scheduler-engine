package com.sos.scheduler.engine.common.scalautil

import java.io.File
import java.nio.charset.Charset
import com.google.common.io.Files

object FileUtils {
  object implicits {
    implicit class RichFile(val delegate: File) extends AnyVal {

      def /(filename: String) =
        new File(delegate, filename)

      def contentString(encoding: Charset) =
        Files.toString(delegate, encoding)

      def write(string: String, encoding: Charset) =
        Files.write(string, delegate, encoding)
    }
  }
}