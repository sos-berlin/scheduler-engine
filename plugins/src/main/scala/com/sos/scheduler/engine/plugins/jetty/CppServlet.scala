package com.sos.scheduler.engine.plugins.jetty

import java.io.OutputStreamWriter
import java.net.URLDecoder
import java.util.concurrent.atomic.AtomicBoolean
import javax.annotation.Nullable
import javax.inject.Inject
import javax.inject.Singleton
import javax.servlet.{AsyncEvent, AsyncListener}
import javax.servlet.http.{HttpServlet, HttpServletRequest, HttpServletResponse}
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
    val operation = getOrSetAttribute(request, classOf[CppServlet].getName) {
      new SchedulerHttpResponse {
        private val waitForNextChunkEvent = new AtomicBoolean(false)
        private var asyncContextInitialized = false
        private val closed = new AtomicBoolean(false)
        private val writer = new OutputStreamWriter(response.getOutputStream, SchedulerConstants.schedulerEncoding)
        /** Das C++-Objekt httpResponseC MUSS mit close() wieder freigegeben werden, sonst Speicherleck. */
        private val httpResponseC = schedulerHttpService.executeHttpRequest(new ServletSchedulerHttpRequest(request), this)
        @Nullable private lazy val chunkReaderC = httpResponseC.chunk_reader

        try start()
        catch { case x => close(); throw x }

        private def start() {
          response.setStatus(httpResponseC.status)
          splittedHeaders(httpResponseC.header_string) foreach { h => response.setHeader(h._1, h._2) }
        }

        def continue() {
          if (chunkReaderC != null) {
            serveChunks()
            if (!isClosed)
              startAsync()
          }
        }

        private def startAsync() {
          val result = request.startAsync()
          if (!asyncContextInitialized) {
            result.setTimeout(0)  // Nie
            result.addListener(new AsyncListener {
              def onComplete(event: AsyncEvent) { close() }
              def onTimeout(event: AsyncEvent) { close() }
              def onError(event: AsyncEvent) { close() }
              def onStartAsync(event: AsyncEvent) {}
            })
            asyncContextInitialized = true
            waitForNextChunkEvent.set(true)
          }
          result
        }

        private def close() {
          if (!closed.getAndSet(true))
            httpResponseC.close()
        }

        def serveChunks() {
          while (!isClosed && chunkReaderC != null && chunkReaderC.next_chunk_is_ready) {
            chunkReaderC.get_next_chunk_size match {
              case 0 => close()
              case size => writer.append(chunkReaderC.read_from_chunk(size))
            }
          }
          writer.flush()
        }

        def onNextChunkIsReady() {
          onThrowableLogOnly(Level.ERROR) {
            if (waitForNextChunkEvent.getAndSet(false))
              request.getAsyncContext.dispatch()
          }
        }

        def isClosed = closed.get
      }
    }

    operation.continue()
  }
}

object CppServlet {
  private implicit val logger = Logger.getLogger(classOf[CppServlet])

  private def splittedHeaders(headers: String) = if (headers.isEmpty) Array() else
    (headers.split("\r\n") map { _.split(": ", 2) } map { h => h(0) -> h(1) })

  class ServletSchedulerHttpRequest(request: HttpServletRequest) extends SchedulerHttpRequest {
    private val _body = CharStreams.toString(request.getReader)

    def hasParameter(name: String) = request.getParameter(name) != null
    def parameter(name: String) = firstNonNull(request.getParameter(name), "")
    def header(name: String) = firstNonNull(request.getHeader(name), "")
    def protocol() = request.getProtocol
    def urlPath = URLDecoder.decode(request.getPathInfo, UTF_8.name) + (if (request.getQueryString != null) "?" else "")
    def charsetName = request.getCharacterEncoding
    def httpMethod = request.getMethod
    def body = _body
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
