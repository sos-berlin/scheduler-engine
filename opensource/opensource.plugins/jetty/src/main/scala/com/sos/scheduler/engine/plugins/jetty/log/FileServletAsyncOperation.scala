package com.sos.scheduler.engine.plugins.jetty.log

import com.google.common.io.ByteStreams.copy
import com.sos.scheduler.engine.kernel.scheduler.SchedulerConstants.logFileEncoding
import java.io._
import java.util.concurrent.atomic.AtomicBoolean
import javax.servlet.http.{HttpServletResponse, HttpServletRequest}
import org.eclipse.jetty.http.HttpException

final class FileServletAsyncOperation(request: HttpServletRequest, response: HttpServletResponse) {

  private var file: File = null
  private var in: FileInputStream = null
  private val woken = new AtomicBoolean
  private var started = false
  private var ended = false

  def start(file: File): Unit = {
    require(!started)
    if (file.toString.isEmpty)  throw new HttpException(javax.servlet.http.HttpServletResponse.SC_BAD_REQUEST, "Log has no file")
    response.setStatus(HttpServletResponse.SC_OK)
    response.setContentType("text/xml;charset="+logFileEncoding)
    response.setHeader("Cache-Control", "no-cache")
    in = new FileInputStream(file)
    started = true
  }

  def close(): Unit = {
    //TODO Sicherstellen, dass close() aufgerufen wird!
    in.close()
  }

  def continue(): Unit = {
    require(started)
    woken.set(false)
    if (!ended && !request.isAsyncStarted) {
      request.startAsync()
    }
    sendFileRemainder()
  }

  def wake(): Unit = {
    require(!ended)
    if (started && !woken.getAndSet(true))
      request.getAsyncContext.dispatch()
  }

  def end(): Unit = {
    if (woken.get)
      ended = true
    else
      request.getAsyncContext.complete()
  }

  private def sendFileRemainder(): Unit = {
    val out = response.getOutputStream
    copy(in, out)
    out.flush()
  }

  override def toString = classOf[FileServletAsyncOperation].getName+"("+file+")"
}
