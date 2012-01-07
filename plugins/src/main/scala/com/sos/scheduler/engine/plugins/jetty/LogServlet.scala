package com.sos.scheduler.engine.plugins.jetty

import java.io._
import java.lang.Math.min
import javax.inject.Inject
import javax.servlet.http.{HttpServletResponse, HttpServletRequest, HttpServlet}
import com.google.inject.Singleton
import com.sos.scheduler.engine.eventbus.EventBus
import com.sos.scheduler.engine.kernel.folder.AbsolutePath
import com.sos.scheduler.engine.kernel.job.JobSubsystem
import com.sos.scheduler.engine.kernel.log.LogSubscription
import com.sos.scheduler.engine.kernel.scheduler.SchedulerConstants.schedulerEncoding
import com.sos.scheduler.engine.kernel.scheduler.SchedulerInstanceId
import org.apache.log4j.Logger
import org.eclipse.jetty.continuation.{Continuation, ContinuationSupport}

@Singleton
//@WebServlet(Array("command"))
class LogServlet @Inject()(jobSubsystem: JobSubsystem, schedulerInstanceId: SchedulerInstanceId, eventBus: EventBus)
extends HttpServlet {
  import LogServlet._

  private val encoding = schedulerEncoding
  private val operationAttributeName = classOf[Operation].getName

  override def doGet(request: HttpServletRequest, response: HttpServletResponse) {
    logger.debug("GET")
    val operation = request.getAttribute(operationAttributeName) match {
      case null =>
        val continuation = ContinuationSupport.getContinuation(request)
        val jobPath = AbsolutePath.of(stripFromEnd(request.getPathInfo, ".job/log"))
        val log = jobSubsystem.job(jobPath).getLog
        val operation = new Operation(response, continuation, log.getFile)
        request.setAttribute(operationAttributeName, operation)
        log.subscribe(new LogSubscription {
          def onLogged() { logger.trace("onLogged"); if (!continuation.isResumed) continuation.resume(); }
          def onClosed() { logger.trace("onClosed"); continuation.complete() }
        })
        //TODO log.unsubscribe(), wenn Continuation endet
        //TODO C++ soll Datei erst nach unsubscribe() löschen (wie macht das der C++-HTTP-Server?)
        operation
      case o: Operation => o
    }

    operation.continuation.suspend()
    operation.sendHeader()
    operation.sendFile()
    //TODO  Erstes onLogged() kann verloren gehen?
  }

  private class Operation(response: HttpServletResponse, val continuation: Continuation, val file: File) {
    var position = 0L

    def sendHeader() {
      response.setStatus(HttpServletResponse.SC_OK)
      response.setHeader("Server", "JobScheduler")
      response.setContentType("text/xml;charset="+encoding)
      response.setHeader("Transfer-Encoding", "chunked")
      //TODO no-cache  response.setHeader("ETag", schedulerInstanceId.getString +"."+byteCount);
    }

    def sendFile() {
      val in = new FileInputStream(file)
      try {
        in.skip(position)
        position += com.google.common.io.ByteStreams.copy(in, response.getOutputStream)
        response.getOutputStream.flush()
      }
      finally in.close()
    }
  }
}

object LogServlet {
  private val logger = Logger.getLogger(classOf[LogServlet])

  private def stripFromEnd(s: String, end: String) = s ensuring {_ endsWith end} substring (0, s.length - end.length)

//  private def copyLimited(input: InputStream, output: OutputStream, limit: Long): Long = {
//    val buffer = new Array[Byte](4096)
//    var written: Long = 0
//    while (written < limit) {
//      val n = input.read(buffer, 0, min(limit - written, buffer.size).toInt)
//      if (n == -1)  return written
//      output.write(buffer, 0, n)
//      written += n
//    }
//    written
//  }
}
