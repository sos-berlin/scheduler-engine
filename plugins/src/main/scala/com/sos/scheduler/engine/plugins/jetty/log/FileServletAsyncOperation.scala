package com.sos.scheduler.engine.plugins.jetty.log

import java.io._
import javax.servlet.http.{HttpServletResponse, HttpServletRequest}
import com.google.common.io.ByteStreams.copy
import com.sos.scheduler.engine.kernel.scheduler.SchedulerConstants.logFileEncoding
import org.apache.log4j.Logger
import java.util.concurrent.atomic.AtomicBoolean
import com.google.common.io.Closeables.closeQuietly

class FileServletAsyncOperation(request: HttpServletRequest, response: HttpServletResponse) {
//  import FileServletAsyncOperation._

  private var file: File = null
  private var in: FileInputStream = null
  private val woken = new AtomicBoolean();
  private var started = false
  private var ended = false

  def start(file: File) {
    require(!started)
    require(!file.toString.isEmpty, "Log has no file")
    response.setStatus(HttpServletResponse.SC_OK)
    response.setContentType("text/xml;charset="+logFileEncoding)
    response.setHeader("Cache-Control", "no-cache")
    in = new FileInputStream(file)
    started = true
  }

  def close() {
    //TODO Sicherstellen, dass close() aufgerufen wird!
    closeQuietly(in)
  }

  def continue() {
    require(started)
    woken.set(false)
    if (!ended && !request.isAsyncStarted) {
      request.startAsync()
    }
    sendFileRemainder()
  }

  def wake() {
    require(!ended)
    if (started && !woken.getAndSet(true))
      request.getAsyncContext.dispatch()
  }

  def end() {
    if (woken.get)
      ended = true
    else
      request.getAsyncContext.complete()
  }

  private def sendFileRemainder() {
    val out = response.getOutputStream
    copy(in, out)
    out.flush()
  }

  override def toString = classOf[FileServletAsyncOperation].getName+"("+file+")"
}

//object FileServletAsyncOperation {
//  private val logger = Logger.getLogger(classOf[FileServletAsyncOperation])
//}
