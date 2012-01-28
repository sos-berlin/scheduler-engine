package com.sos.scheduler.engine.plugins.jetty

import java.net.URLDecoder
import java.util.concurrent.atomic.AtomicBoolean
import javax.annotation.Nullable
import javax.inject.Inject
import javax.inject.Singleton
import javax.servlet.{AsyncEvent, AsyncListener}
import javax.servlet.http.{HttpServlet, HttpServletRequest, HttpServletResponse}
import com.sos.scheduler.engine.cplusplus.runtime.CppReference
import com.sos.scheduler.engine.kernel.http.{SchedulerHttpRequest, SchedulerHttpResponse}
import com.sos.scheduler.engine.kernel.scheduler.{SchedulerHttpService, SchedulerConstants}
import com.sos.scheduler.engine.plugins.jetty.WebServiceFunctions.getOrSetAttribute
import com.google.common.base.Charsets._
import com.google.common.base.Objects._
import com.google.common.io.CharStreams
import org.apache.log4j.{Level, Logger}

@Singleton
class CppServlet @Inject()(schedulerHttpService: SchedulerHttpService) extends HttpServlet {
  import CppServlet._

  override def service(request: HttpServletRequest, response: HttpServletResponse) {
    val operation = getOrSetAttribute(request, classOf[CppServlet].getName) { newOperation(request, response) }
    try operation.continue()
    catch { case x => operation.close(); throw x }
  }

  private def newOperation(request: HttpServletRequest, response: HttpServletResponse) = new SchedulerHttpResponse {
    private val _isClosed = new AtomicBoolean(false)

    private val asyncListener = new AsyncListener {
      def onComplete(event: AsyncEvent) {
        close()
      }
      def onTimeout(event: AsyncEvent) {
        close()
      }
      def onError(event: AsyncEvent) {
        for (t <- Option(event.getThrowable)) logger.error("AsyncListener.onError: "+t, t)
        close()
      }
      def onStartAsync(event: AsyncEvent) {}
    }

    /** Das C++-Objekt httpResponseC MUSS mit Release() wieder freigegeben werden, sonst Speicherleck. */
    private lazy val httpResponseC = schedulerHttpService.executeHttpRequest(new ServletSchedulerHttpRequest(request), this)
    private lazy val httpResponseCRef = CppReference.of(httpResponseC)
    @Nullable private lazy val chunkReaderC = httpResponseC.chunk_reader

    try {
      response.setStatus(httpResponseC.status)
      splittedHeaders(httpResponseC.header_string) foreach { h => response.setHeader(h._1, h._2) }
    }
    catch { case x => close(); throw x }

    def close() {
      if (!_isClosed.getAndSet(true)) {
        httpResponseC.close()
        logger.debug("httpResponseC.dispose()")
        httpResponseCRef.dispose()
      }
    }

    def continue() {
      if (chunkReaderC != null) {
        serveChunks()
        if (!isClosed) startAsync()
        else response.getOutputStream.flush()
      } else
        logger.warn("chunkReaderC==null")
    }

    private def startAsync() {
      val result = request.startAsync(request, response)
      result.setTimeout(0)  // Nie
      result.addListener(asyncListener)
      result
    }

    def serveChunks() {
      while (!isClosed && chunkReaderC != null && chunkReaderC.next_chunk_is_ready) {
        chunkReaderC.get_next_chunk_size match {
          case 0 => close()
          case size => response.getOutputStream.write(chunkReaderC.read_from_chunk(size))
        }
      }
      response.getOutputStream.flush()
    }

    def onNextChunkIsReady() {
      onThrowableLogOnly(Level.ERROR) {
        if (request.isAsyncStarted)
          request.getAsyncContext.dispatch()
      }
    }

    final def isClosed = _isClosed.get
  }
}

object CppServlet {
  private implicit val logger: Logger = Logger.getLogger(classOf[CppServlet])

  private def splittedHeaders(headers: String): Iterable[(String,String)] = headers match {
    case "" => Iterable()
    case _ => headers.split("\r\n") map { _.split(": ", 2) } map { h => h(0) -> h(1) }
  }

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

  def onThrowableLogOnly(level: Level)(f: => Unit)(implicit log: Logger) {
    onThrowableLogOnly(log, level, f)
  }

  def onThrowableLogOnly(log: Logger, level: Level, f: => Unit) {
    try f
    catch {
      case x => logger.log(level, x, x)
    }
  }
}
