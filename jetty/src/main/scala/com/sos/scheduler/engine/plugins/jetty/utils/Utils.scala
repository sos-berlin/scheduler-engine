package com.sos.scheduler.engine.plugins.jetty.utils

import javax.servlet.ServletRequest

object Utils {
  def getOrSetAttribute[A](request: ServletRequest, attributeName: String)(f: => A) =
    Option(request.getAttribute(attributeName).asInstanceOf[A]) getOrElse {
      val result = f
      request.setAttribute(attributeName, result)
      result
    }
}
