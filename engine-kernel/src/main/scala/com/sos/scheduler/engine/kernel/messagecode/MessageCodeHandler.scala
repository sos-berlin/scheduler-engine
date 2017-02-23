package com.sos.scheduler.engine.kernel.messagecode

import com.sos.jobscheduler.data.message.MessageCode
import com.sos.scheduler.engine.kernel.messagecode.MessageCodeHandler._
import scala.collection.mutable

/**
 * Turns a MessageCode with insertions into a human readable string
 *
 * @author Joacim Zschimmer
 */
final class MessageCodeHandler private(codeToTemplate: MessageCode ⇒ Template) {

  def apply(code: MessageCode, insertions: Any*): String = toString(code, insertions.toIndexedSeq)

  def toString(code: MessageCode, insertions: IndexedSeq[Any]): String =
    (code + " " + codeToTemplate(code).apply(insertions map { _.toString })).trim
}

object MessageCodeHandler {
  /**
   * Insertions are clipped to this value.
   */
  val InsertionLengthMaximum = 500

  /**
   * @param codesAndTexts Strings of pattern "code textWithInsertions", every insertion denoted by "$" + index
   */
  def fromCodeAndTextStrings(codesAndTexts: Iterable[String]): MessageCodeHandler = {
    val codesAndTemplates = codesAndTexts map { codeAndText ⇒
      val Array(code, text) = codeAndText.split(" ", 2)
      MessageCode(code) → new Template(text)
    }
    new MessageCodeHandler(codesAndTemplates.toMap withDefaultValue new Template(""))
  }

  private class Template(raw: String) {
    lazy val segments: List[Segment] = {
      val parts = (raw split "\\$").toList
      TextSegment(parts.head) :: (parts.tail flatMap {
        case "" ⇒ Nil
        case s ⇒ s.head match {
          case d if d >= '1' && d <= '9' ⇒ InsertionSegment(d - '1') :: (List(s.tail) filter { _.nonEmpty } map TextSegment)
          case '$' ⇒ TextSegment(s) :: Nil
          case c ⇒ TextSegment("$" + s) :: Nil
        }
      })
    }

    def apply(insertions: IndexedSeq[String]): String = {
      def clipped(i: Int) = insertions(i) take InsertionLengthMaximum
      val usedIndices = mutable.BitSet()
      val strings = segments collect {
        case TextSegment(string) ⇒ string
        case InsertionSegment(index) ⇒
          if (index < insertions.size) {
            usedIndices += index
            clipped(index)
          } else
            "$" + (index + 1)
      }
      val unusedInsertions = insertions.indices filterNot usedIndices map clipped map { o ⇒ s" [$o]" } mkString ""
      ((strings ++ unusedInsertions) mkString "").trim
    }
  }

  private trait Segment
  private case class TextSegment(string: String) extends Segment
  private case class InsertionSegment(index: Int) extends Segment
}
