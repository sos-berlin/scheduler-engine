package com.sos.scheduler.engine.tests.jira.js795

import javax.servlet.http.{HttpServletResponse, HttpServletRequest, HttpServlet}
import com.google.inject.Singleton
import com.sos.scheduler.engine.kernel.Scheduler
import com.google.common.base.Charsets._
import com.google.common.io.CharStreams
import java.io.OutputStreamWriter
import javax.inject.Inject

@deprecated("", "")
@Singleton
//@WebServlet(Array("command"))
class CommandServlet @Inject()(scheduler: Scheduler) extends HttpServlet {
  import CommandServlet._

  override def doGet(request: HttpServletRequest, response: HttpServletResponse) {
    val command = Option(request.getParameter("command")) getOrElse {
      throw new RuntimeException("Missing parameter command")
    }
    executeCommandAndRespond(response, command)
    //TODO XML-Antwort lesbar einrücken
  }

  override def doPost(request: HttpServletRequest, response: HttpServletResponse) {
    val commandXml = CharStreams.toString(request.getReader)
    executeCommandAndRespond(response, commandXml)
  }

  private def executeCommandAndRespond(response: HttpServletResponse, command: String) {
    if (command.isEmpty) throw new RuntimeException("Missing command");
    val responseXml = scheduler.executeXml(command)
    response.setHeader("Cache-Control", "no-cache");
    sendXmlResponse(response, responseXml)
  }

  private def sendXmlResponse(response: HttpServletResponse, responseXml: String) {
    response.setStatus(HttpServletResponse.SC_OK)
    response.setContentType("text/xml;charset=" + responseEncoding)
    val writer = new OutputStreamWriter(response.getOutputStream, responseEncoding)
    writer.append(responseXml).append(testString)
    writer.flush()
  }
}

object CommandServlet {
  private val responseEncoding = UTF_8
  val testString = "<!--Jetty test-->"
}
