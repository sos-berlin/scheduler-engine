package com.sos.scheduler.engine.common.xml

import com.sos.scheduler.engine.common.scalautil.xmls.ScalaXMLEventReader
import javax.xml.stream.XMLEventReader
import scala.collection.immutable.IndexedSeq
import scala.xml.{TopScope, UnprefixedAttribute}

/**
 * @author Joacim Zschimmer
 */
object VariableSets {

  private val DefaultElementName = "variable"

  def toParamsXmlElem(variables: Iterable[(String, String)]) = toXmlElem(variables, "params", "param")

  def toXmlElem(
      variables: Iterable[(String, String)],
      elementName: String = "sos.spooler.variable_set",
      subelementName: String = "variable"): xml.Elem = {

    val children = variables map { case (k, v) ⇒
      xml.Elem(
        prefix = null,
        label = subelementName,
        attributes = new UnprefixedAttribute("name", xml.Text(k),
          if (v exists { c ⇒ !hex.isLatin1(c) })
            new UnprefixedAttribute("hex_value", xml.Text(hex.bytesToHex(k, v)), xml.Null)
          else
            new UnprefixedAttribute("value", xml.Text(v), xml.Null)
        ),
          scope = TopScope,
        minimizeEmpty = true)
    }
    xml.Elem(null: String, elementName, xml.Null, TopScope, children.isEmpty, children.toSeq: _*)
  }

  def parseXml(string: String): Map[String, String] = parseXml(string, groupName = "", elementName = DefaultElementName)

  def parseXml(string: String, groupName: String, elementName: String): Map[String, String] =
    ScalaXMLEventReader.parseString(string) { eventReader ⇒ parseXml(eventReader, groupName, elementName) }

  def parseXml(xmlEventReader: XMLEventReader, groupName: String, elementName: String): Map[String, String] = {
    val eventReader = new ScalaXMLEventReader(xmlEventReader)
    import eventReader._
    val myGroupName = if (groupName.nonEmpty) groupName else peek.asStartElement.getName.getLocalPart
    parseElement(myGroupName) {
      attributeMap.ignore("count")
      parseEachRepeatingElement(elementName) {
        attributeMap("name") →
          (attributeMap.get("value") orElse (attributeMap.get("hex_value") map hex.hexToBytes) getOrElse "")
      }
    } .toMap
  }

  private object hex {
    val isLatin1: IndexedSeq[Boolean] = Vector(
    //  0  1  2  3  4  5  6  7  8  9  a  b  c  d  e  f
        0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 0, 1, 0, 0, 0,     // 00  nur \t \n \r
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,     // 10
        1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,     // 20
        1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,     // 30
        1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,     // 40
        1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,     // 50
        1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,     // 60
        1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0,     // 70  \x7f nicht
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,     // 80
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,     // 90
        1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,     // a0
        1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,     // b0
        1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,     // c0
        1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,     // d0
        1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,     // e0
        1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1      // f0
    ) map { _ != 0 }

    def bytesToHex(key: String, value: String): String =
      (value map { c ⇒
        val i = c.toInt
        if (c < 0 || c >= 0x100) throw new IllegalArgumentException(f"Variable '$key' contains a non-8-bit character: '$c' U+$i%04X")
        f"$i%02x"
      }).mkString

    def hexToInt(c: Char) =
      if (c >= '0' && c <= '9')
        c - '0'
      else if (c >= 'a' && c <= 'f')
        c - ('a' + 10 - 1)
      else
        throwInvalidHexValue()


    def hexToBytes(value: String): String = {
      if(value.length % 2 != 0) throwInvalidHexValue()
      (for (i ← 0 until value.length by 2) yield
        ((hexToInt(value(i)) << 4) | hexToInt(value(i + 1))).toChar
      ).mkString
    }

    private def throwInvalidHexValue() = throw new IllegalArgumentException("""Invalid <variable "hex_value"=...>""")
  }
}
