package com.sos.scheduler.engine.plugins.jetty

import com.google.common.base.Charsets._
import com.google.common.base.Objects._
import com.google.common.io.CharStreams
import com.sos.scheduler.engine.kernel.http.{SchedulerHttpRequest, SchedulerHttpResponse}
import com.sos.scheduler.engine.plugins.jetty.WebServiceFunctions.getOrSetAttribute
import java.net.URLDecoder
import java.util.concurrent.atomic.AtomicBoolean
import javax.annotation.Nullable
import javax.inject.Inject
import javax.inject.Singleton
import javax.servlet.{AsyncEvent, AsyncListener}
import javax.servlet.http.{HttpServlet, HttpServletRequest, HttpServletResponse}
import org.slf4j.{Logger, LoggerFactory}
import com.sos.scheduler.engine.cplusplus.runtime.{CppProxyInvalidatedException, DisposableCppProxyRegister}
import com.sos.scheduler.engine.kernel.scheduler.{SchedulerIsClosed, SchedulerHttpService}

@Singleton
class CppServlet @Inject()(
    schedulerHttpService: SchedulerHttpService,
    disposableCppProxyRegister: DisposableCppProxyRegister,
    schedulerIsClosed: SchedulerIsClosed)
    extends HttpServlet {

  import CppServlet._

  override def service(request: HttpServletRequest, response: HttpServletResponse) {
    val operation = getOrSetAttribute(request, classOf[CppServlet].getName) { newOperation(request, response) }
    try operation.continueOrClose()
    catch {
      case x: InterruptedException => logger.debug(x.toString)
      case x: RuntimeException if x.getCause.isInstanceOf[InterruptedException] => logger.debug(x.toString)
      case x: CppProxyInvalidatedException => logger.debug(x.toString)
    }
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

    //FIXME Wenn httpResponseC erst nach Scheduler-Ende freigegeben wird, gibt's einen Absturz, weil Spooler freigegeben ist.
    /** Das C++-Objekt httpResponseC MUSS mit Release() wieder freigegeben werden, sonst Speicherleck. */
    private lazy val httpResponseCRef = disposableCppProxyRegister.reference(schedulerHttpService.executeHttpRequest(new ServletSchedulerHttpRequest(request), this))
    private def httpResponseC = httpResponseCRef.get
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
        disposableCppProxyRegister.dispose(httpResponseCRef)
      }
    }

    def tryClose() {
      try close()
      catch {
        case x =>
          if (schedulerIsClosed.isClosed) logger.error(x.toString)
          else logger.error(x.toString, x)
      }
    }

    def continueOrClose() {
      try continue()
      catch {
        case x =>
          tryClose()
          throw x
      }
    }

    def continue() {
      if (chunkReaderC != null) {
        serveChunks()
        response.getOutputStream.flush()
        if (!isClosed) startAsync()
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
    }

    def onNextChunkIsReady() {
      onThrowableLogOnly {
        if (request.isAsyncStarted)
          request.getAsyncContext.dispatch()
      }
    }

    final def isClosed = _isClosed.get
  }
}

object CppServlet {
  private implicit val logger: Logger = LoggerFactory.getLogger(classOf[CppServlet])

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

//  def onThrowable[A](f: => A) = new {
//    def rollback(r: => Unit) = {
//      try f
//      catch {
//        case x =>
//          try r
//          catch {
//            case rollbackThrowable =>
//              logger.error(x.toString, x)
//              throw rollbackThrowable
//          }
//          throw x
//      }
//    }
//  }

  def onThrowableLogOnly(f: => Unit)(implicit log: Logger) {
    onThrowableLogOnly(log, f)
  }

  def onThrowableLogOnly(log: Logger, f: => Unit) {
    try f
    catch {
      case x => logger.error(x.toString, x)
    }
  }
}
