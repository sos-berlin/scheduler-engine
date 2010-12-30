package com.sos.scheduler.kernel.cplusplus.generator.util

import java.io.File
import java.nio.charset.Charset
import org.apache.commons.io.FileUtils
import scala.xml._
import scala.xml.NodeSeq._


object XmlUtil {
    def replaceNodes(node: Node)(f: PartialFunction[Node, NodeSeq]): NodeSeq = {
        def t(node: Node): NodeSeq = node match {
            case n if (f isDefinedAt n) => f(n)
            case e: Elem if e.descendant exists f.isDefinedAt => e.copy(e.prefix, e.label, e.attributes, e.scope, e.child flatMap t)
            case n => n
        }
        t(node)
    }

    private lazy val utf8 = Charset.forName("UTF-8")

    def xmlOf(e: Elem, encoding: Charset = utf8) =
        "<?xml version=\"1.0\" encoding=\"" + encoding.name + "\"?>\n" +
        correctNewlinesInAttributesForScala(e.toString)

//    def nodeAreEqualIgnoringSpace(node: NodeSeq, n: NodeSeq, iterableComparison: Function2[NodeSeq, NodeSeq, Boolean]) =
//        (node, n) match {
//            case (a: Atom[_], b: Atom) => a.text.trim == b.text.trim
//            case (a: Node, b: Node) =>
//                nodeIsSpace(a) && nodeIsSpace(b) ||
//                a.prefix == b.prefix &&
//                a.attributes == b.attributes &&
//                a.label == b.label &&
//                iterableComparison(a.child filterNot nodeIsSpace, b.child filterNot nodeIsSpace)
//            case (n1: NodeSeq, n2: NodeSeq) => iterableComparison(n1 filterNot nodeIsSpace, n2 filterNot nodeIsSpace)
//    }
//
//    def nodeIsSpace(n: Node) = n match {
//        case g: Group => false
//        case _ => n.label.equals("#PCDATA")  &&  stringIsSpace(n.text)
//    }
//
//    private val spacesRegex = """^\s*$""".r
//
//    private def stringIsSpace(s: String) = spacesRegex.findFirstMatchIn(s).isDefined


    
    def saveAndCorrectScalaBug(file: File, document: Elem, encoding: Charset) {
        FileUtils.writeStringToFile(file, xmlOf(document, encoding), encoding.name)
    }

    /** Scala 2.8.0 schreibt Zeilenwechsel in Attributen uncodiert. Das ist falsch, Zeilenwechsel muss als &#10; geschrieben werden.
     * (Ein eigener Serialisierer wäre wohl klarer.)
     */
    def correctNewlinesInAttributesForScala(xml: String) = modifyAttributeCharacters(xml)(
        Map('\r' -> "&#x0D;", '\n' -> "&#x0A;")
    )

    def modifyAttributeCharacters(xml: String)(f: PartialFunction[Char, String]) = {
        val result = new StringBuilder(xml.length * 11 / 10)
        var inElement = false

        def eatComment(pos: Int) = {
            val end = xml.indexOf("-->", pos)
            if (end == -1)  throw new RuntimeException("Open XML comment at position " + pos)
            result.append(xml.slice(pos, end+3))
            end + 3
        }

        def eatQuotedAttribute(pos: Int) = {
            val quote = xml(pos)
            result.append(quote)
            var i = pos + 1
            while (i < xml.length  &&  xml(i) != quote) {
                val c = xml(i)
                result.append(if (f isDefinedAt c) f(c)  else c.toString)
                i += 1
            }
            if (i < xml.length) {
                result.append(quote)
                i += 1
            }
            i
        }

        var i = 0
        while (i < xml.length) {
            xml(i) match {
                case '<' if i + 2 < xml.length && xml(i+1) == '-'  &&  xml(i+2) == '-' =>
                    i = eatComment(i)
                case c @ '<' =>
                    inElement = true
                    result.append(c)
                    i += 1
                case c @ '>' if inElement =>
                    inElement = false
                    result.append(c)
                    i += 1
                case c @ ('"' | '\\') if inElement =>
                    i = eatQuotedAttribute(i)
                case c =>
                    result.append(c)
                    i += 1
            }
        }

        result.toString
    }
}
