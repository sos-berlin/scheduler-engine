package com.sos.scheduler.engine.plugins.jetty

import javax.servlet.{AsyncEvent, AsyncListener}
import javax.servlet.http.{HttpServletResponse, HttpServletRequest}
import com.sos.scheduler.engine.kernel.log.{PrefixLog, LogSubscription}
import org.apache.log4j.Logger

//TODO Datei erst nach unsubscribe() löschen.
//Weil Prefix_log zerstört ist, kann vielleicht die Verantwortung fürs Löschen an Java übergehen.

object LogServletAsyncOperation {
  private val logger = Logger.getLogger(getClass)

  def apply(request: HttpServletRequest, response: HttpServletResponse, log: PrefixLog) = {
    val operation = new FileServletAsyncOperation(request, response)

    val logSubscription = new LogSubscription {
      def onStarted() {
        logger.info("onStarted")
        operation.start(log.getFile)
      }
      def onClosed() {
        logger.info("onClosed")
        operation.end()
        // Unter Windows kann jetzt der Fehler SCHEDULER-291 kommen, weil die Datei noch vom Servlet geöffnet ist.
      }
      def onLogged() {
        operation.wake()
      }
    }

    log.subscribe(logSubscription)

    val asyncContext = request.startAsync()
    asyncContext.setTimeout(0)  // 0: Nie

    asyncContext.addListener(new AsyncListener {
      def onStartAsync(event: AsyncEvent) {}
      def onComplete(event: AsyncEvent) { close() }
      def onTimeout(event: AsyncEvent) { close() }
      def onError(event: AsyncEvent) {
        logger.error(event.getThrowable, event.getThrowable)
        close()
      }
      private def close() {
        log.unsubscribe(logSubscription)
        operation.close()
      }
    })

    if (log.isStarted)
      operation.start(log.getFile)
    operation
  }
}
