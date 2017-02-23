package com.sos.scheduler.engine.playground.zschimmer.cpptoutf8

import com.google.common.base.Charsets.UTF_8
import com.google.common.base.Charsets._
import com.sos.jobscheduler.common.scalautil.FileUtils.implicits.RichFile
import com.sos.jobscheduler.common.scalautil.Logger
import com.sos.scheduler.engine.cplusplus.scalautil.io.FileUtil.writingFileIfDifferent
import java.io.{FilenameFilter, Writer, File}
import java.nio.charset.{CodingErrorAction, CoderResult}
import java.nio.{CharBuffer, ByteBuffer}
import scala.annotation.tailrec
import scala.collection.immutable

/** Konvertiert C- und C++-Quellcode zu UTF-8. */
private object CppToUtf8 {

  private val extensions = immutable.Seq(".cxx", ".c", ".h", ".odl")
  private val logger = Logger(getClass)
  private val lineEndCommentStart = "//".getBytes(US_ASCII)
  private val commentStart = "/*".getBytes(US_ASCII)
  private val commentEnd = "*/".getBytes(US_ASCII)

  def convertFileOrDirectory(file: File): Unit = {
    if (file.isDirectory) {
      val myFilter = new FilenameFilter {
        def accept(dir: File, name: String) = (extensions exists name.endsWith) || new File(dir, name).isDirectory
      }
      file.listFiles(myFilter) foreach convertFileOrDirectory
    }
    else {
      try {
        val converted = convertFile(file)
        if (converted) logger.info(file.toString)
      }
      catch {
        case e: Exception ⇒ logger.error(s"$file: $e")
      }
    }
  }

  def convertFile(file: File): Boolean =
    writingFileIfDifferent(file, UTF_8) { writer ⇒
      convertBytes(file.contentBytes, writer)
    }

  private[cpptoutf8] def convertBytes(bytes: Seq[Byte], writer: Writer): Unit = {
    for (part <- inputStreamToParts(bytes))
      writer.write(part.convertToString)
  }

  private def inputStreamToParts(bytes: Seq[Byte]): Iterator[Part] =
    new Iterator[Part] {
      var index = 0
      var inComment = false
      var inLineEndComment = false

      def hasNext = index < bytes.size

      def next() = {
        if (inLineEndComment) {
          val end = bytes.indexOf('\n', index) match {
            case -1 ⇒ bytes.length
            case i ⇒ i + 1
          }
          val result = ConvertablePart(bytes.slice(index, end))
          index = end
          inLineEndComment = false
          result
        } else
        if (inComment) {
          val end = bytes.indexOfSlice(commentEnd, index) match {
            case -1 ⇒ bytes.length
            case i ⇒ i + commentEnd.length
          }
          val result = ConvertablePart(bytes.slice(index, end))
          index = end
          inComment = false
          result
        } else {
          val (i, c) = index to bytes.length - 2 collectFirst {
            case i: Int if bytes(i) == '/' && (bytes(i+1) == '/' || bytes(i+1) == '*') ⇒
              i -> bytes(i+1)
          } getOrElse bytes.length -> '!'
          c match {
            case '/' ⇒ inLineEndComment = true
            case '*' ⇒ inComment = true
            case '!' ⇒
          }
          val result = InconvertablePart(bytes.slice(index, i))
          index = i
          result
        }
      }
    }

  private trait Part {
    val bytes: Seq[Byte]
    def convertToString: String
    override def toString = convertToString
  }

  private case class InconvertablePart(bytes: Seq[Byte]) extends Part {
    for (b <- bytes map { _.toInt & 0xFF })
      require(b < 0x7F || b == '\n' || b == '\r' || b == '\t', f"Non-ASCII character in code: '${b.toChar}' 0x$b%02x")
    def convertToString = new String(bytes.toArray, ISO_8859_1)
  }

  private case class ConvertablePart(bytes: Seq[Byte]) extends Part {
    def convertToString = convertBytesToString(bytes)
  }

  private def convertBytesToString(bytes: Seq[Byte]): String = {
    val utf8Decoder = UTF_8.newDecoder().onMalformedInput(CodingErrorAction.REPORT).onUnmappableCharacter(CodingErrorAction.REPORT)
    val byteBuffer = ByteBuffer.wrap(bytes.toArray)
    val charBuffer = CharBuffer.allocate(bytes.size)

    @tailrec def f(): String =
      utf8Decoder.decode(byteBuffer, charBuffer, false) match {
//        case coderResult if coderResult.isMalformed || coderResult.isUnmappable ⇒
//          for (i <- -coderResult.length until 0)
//            charBuffer.put(bytes(byteBuffer.position + i).toChar)
//          f()
        case _ if byteBuffer.hasRemaining ⇒
          charBuffer.put((byteBuffer.get() & 0xFF).toChar)
          f()
        case CoderResult.UNDERFLOW if !byteBuffer.hasRemaining ⇒
          new String(charBuffer.array(), 0, charBuffer.position)
        case coderResult ⇒
          coderResult.throwException().asInstanceOf[Nothing]
      }
    f()
  }
}
