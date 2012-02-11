package com.sos.scheduler.engine.plugins.jetty.cpp

import javax.servlet.http.HttpServletRequest
import com.sos.scheduler.engine.kernel.http.SchedulerHttpRequest
import com.google.common.base.Objects._
import com.google.common.base.Charsets._
import com.google.common.io.CharStreams
import java.net.URLDecoder

class ServletSchedulerHttpRequest(request: HttpServletRequest) extends SchedulerHttpRequest {
  def hasParameter(name: String) = request.getParameter(name) != null

  def parameter(name: String) = firstNonNull(request.getParameter(name), "")

  def header(name: String) = firstNonNull(request.getHeader(name), "")

  def protocol() = request.getProtocol

  def urlPath = URLDecoder.decode(firstNonNull(request.getPathInfo, ""), UTF_8.name) + (if (request.getQueryString != null) "?" else "")

  def charsetName = request.getCharacterEncoding

  def httpMethod = request.getMethod

  val body = CharStreams.toString(request.getReader)
}
