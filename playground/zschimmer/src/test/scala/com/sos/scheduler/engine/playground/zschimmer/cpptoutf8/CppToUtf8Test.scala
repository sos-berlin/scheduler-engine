package com.sos.scheduler.engine.playground.zschimmer.cpptoutf8

import CppToUtf8._
import com.google.common.base.Charsets.{ISO_8859_1, UTF_8}
import com.sos.scheduler.engine.common.scalautil.FileUtils.implicits.RichFile
import com.sos.scheduler.engine.common.scalautil.Logger
import java.io.{StringWriter, File}
import org.junit.runner.RunWith
import org.scalatest.FunSuite
import org.scalatest.Matchers._
import org.scalatest.junit.JUnitRunner

@RunWith(classOf[JUnitRunner])
final class CppToUtf8Test extends FunSuite {

  test("convertLine empty") {
    check("" -> "")
  }

  test("convertLine ASCII") {
    check("x" -> "x")
    check("x // y" -> "x // y")
    check("// y" -> "// y")
  }

  test("Non-ASCII only in comment") {
    intercept[IllegalArgumentException] {
      convertToString(Seq(0xE4.toByte))
    }
  }

  test("convertLine ISO-8859-1") {
    check("//xxä-ö-ü" -> "//xxä-ö-ü")
    check("//xxäöü" -> "//xxäöü")
  }

  test("convertLine mixed ISO-8859-1 and UTF-8") {
    check("//HÃ¶he Höhe" -> "//Höhe Höhe")
  }

  test("Multi-line comment") {
    check("xx\n/*HÃ¶he Höhe\n*/yy" -> "xx\n/*Höhe Höhe\n*/yy")
    intercept[IllegalArgumentException] {
      convertToString(toBytes("xx\n/*yy\n*/yyHöhe"))
    }
  }

  test("file") {
    val file = File.createTempFile("test", ".tmp")
    file.deleteOnExit()
    file.contentBytes = toBytes("//xxä-ö-ü\n" + "//HÃ¶he Höhe\n")
    convertFile(file)
    file.contentString(UTF_8) shouldEqual ("//xxä-ö-ü\n" + "//Höhe Höhe\n")
  }

  private def check(o: (String, String)) {
    convertToString(toBytes(o._1)) shouldEqual o._2
  }

  private def toBytes(o: String) =
    o.getBytes(ISO_8859_1)

  private def convertToString(o: Seq[Byte]): String = {
    val w = new StringWriter
    convertBytes(o, w)
    w.toString
  }
}

private object CppToUtf8Test {
  private val logger = Logger(getClass)
}
