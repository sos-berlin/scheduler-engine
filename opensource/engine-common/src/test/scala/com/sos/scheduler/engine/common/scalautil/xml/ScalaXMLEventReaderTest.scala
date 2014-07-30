package com.sos.scheduler.engine.common.scalautil.xml

import com.sos.scheduler.engine.common.scalautil.ScalaUtils._
import com.sos.scheduler.engine.common.scalautil.xml.ScalaXMLEventReader._
import com.sos.scheduler.engine.common.scalautil.xml.ScalaXMLEventReaderTest._
import org.junit.runner.RunWith
import org.scalatest.FreeSpec
import org.scalatest.Matchers._
import org.scalatest.junit.JUnitRunner
import scala.collection.immutable

/**
 * @author Joacim Zschimmer
 */
@RunWith(classOf[JUnitRunner])
final class ScalaXMLEventReaderTest extends FreeSpec {
  "ScalaXMLEventReader" in {
    val testXmlString = <A><B/><C x="xx" optional="oo"><D/><D/></C></A>.toString()
    parseString(testXmlString)(parseA) shouldEqual A(B(), C(x = "xx", o = "oo", List(D(), D())))
  }

  "Optional attribute" in {
    val testXmlString = <A><B/><C x="xx"><D/><D/></C></A>.toString()
    parseString(testXmlString)(parseA) shouldEqual A(B(), C(x = "xx", o = "DEFAULT", List(D(), D())))
  }

  "Detects extra attribute" in {
    val testXmlString = <A><B/><C x="xx" optional="oo" z="zz"><D/><D/></C></A>.toString()
    intercept[WrappedException] { parseString(testXmlString)(parseA) }
      .rootCause.asInstanceOf[UnparsedAttributesException].names shouldEqual List("z")
  }

  "Detects missing attribute" in {
    val testXmlString = <A><B/><C><D/><D/></C></A>.toString()
    intercept[WrappedException] { parseString(testXmlString)(parseA) }
      .rootCause.asInstanceOf[NoSuchElementException]
  }

  "Detects extra element" in {
    val testXmlString = <A><B/><C x="xx"><D/><D/></C><EXTRA/></A>.toString()
    intercept[WrappedException] { parseString(testXmlString)(parseA) }
  }

  "Detects extra repeating element" in {
    val testXmlString = <A><B/><C x="xx"><D/><D/><EXTRA/></C></A>.toString()
    intercept[WrappedException] { parseString(testXmlString)(parseA) }
  }

  "Detects missing element" in {
    val testXmlString = <A><C x="xx"><D/><D/></C></A>.toString()
    intercept[Exception] { parseString(testXmlString)(parseA) }
      .rootCause.asInstanceOf[NoSuchElementException]
  }
}

private object ScalaXMLEventReaderTest {
  private case class A(b: B, c: C)
  private case class B()
  private case class C(x: String, o: String, ds: immutable.Seq[D])
  private case class D()

  private def parseA(eventReader: ScalaXMLEventReader): A = {
    import eventReader._

    def parseC(): C =
      parseElement("C") {
        val x = attributeMap("x")
        val o = attributeMap.getOrElse("optional", "DEFAULT")
        val ds = parseEachRepeatingElement("D") { D() }
        C(x, o, ds.to[immutable.Seq])
      }

    parseElement("A") {
      val elementsMap = forEachStartElement {
        case "B" ⇒ parseElement() { B() }
        case "C" ⇒ parseC()
      }
      (elementsMap.one[B]("B"): B) shouldEqual (elementsMap.one[B]: B)
      (elementsMap.one[C]("C"): C) shouldEqual (elementsMap.one[C]: C)
      A(elementsMap.one[B]("B"), elementsMap.one[C])
    }
  }
}
