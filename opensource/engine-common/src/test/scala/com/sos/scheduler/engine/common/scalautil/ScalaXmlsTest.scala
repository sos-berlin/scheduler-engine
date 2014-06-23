package com.sos.scheduler.engine.common.scalautil

import java.io.File

import org.scalatest.FreeSpec
import org.scalatest.Matchers._
import ScalaXmls.implicits._
import com.google.common.base.Charsets.{UTF_8, ISO_8859_1}
import com.google.common.io.Files

/**
 * @author Joacim Zschimmer
 */
final class ScalaXmlsTest extends FreeSpec {
  ".xml" in {
    val f = File.createTempFile("sos", ".tmp")
    try {
      f.xml = <å/>
      f.xml shouldEqual <å/>
      Files.toString(f, UTF_8) shouldEqual "<?xml version='1.0' encoding='UTF-8'?>\n<å/>"
    }
    finally f.delete()
  }

  ".toBytes" in {
    <å/>.toBytes shouldEqual "<?xml version='1.0' encoding='UTF-8'?><å/>".getBytes(UTF_8)
  }

  ".toBytes xmlDecl=false" in {
    <å/>.toBytes(xmlDecl = false, encoding = ISO_8859_1) shouldEqual """<å/>""".getBytes(ISO_8859_1)
  }
}
