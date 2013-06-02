package com.sos.scheduler.engine.plugins.jetty.utils

import javax.servlet.ServletRequest
import scala.math._
import scala.util.Random

object Utils {
  def randomInts(range: Range) = {
    val r = randomInt(range)
    (r to range.last).toIterator ++ (range.head until r).toIterator
  }

  def randomInt(r: Range) =
    r.head + abs(Random.nextInt()) % (r.last - r.head + 1)

  def getOrSetAttribute[A](request: ServletRequest, attributeName: String)(f: => A) =
    Option(request.getAttribute(attributeName).asInstanceOf[A]) getOrElse {
      val result = f
      request.setAttribute(attributeName, result)
      result
    }
}
