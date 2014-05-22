package com.sos.scheduler.engine.tests.jira.js973

import java.io.Reader
import scala.sys._

final class ReaderIterator(reader: Reader) extends Iterator[Char] {
  private var nextChar: Int = -2

  def hasNext =
    provideNextByte() != -1

  def next() = {
    val result = provideNextByte()
    if (result < 0)  error("End of InputStream")
    nextChar = -2
    result.toChar
  }

  @inline private def provideNextByte() = {
    if (nextChar == -2)
      nextChar = reader.read()
    nextChar
  }
}
