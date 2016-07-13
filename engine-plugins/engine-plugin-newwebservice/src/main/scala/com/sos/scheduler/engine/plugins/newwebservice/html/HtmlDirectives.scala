package com.sos.scheduler.engine.plugins.newwebservice.html

import com.sos.scheduler.engine.plugins.newwebservice.html.HtmlPage._
import scala.concurrent.{ExecutionContext, Future}
import scala.language.implicitConversions
import spray.http.HttpHeaders.Accept
import spray.http.MediaRange
import spray.http.MediaTypes.`text/html`
import spray.httpx.marshalling.ToResponseMarshallable
import spray.routing.Directives._
import spray.routing._

/**
  * @author Joacim Zschimmer
  */
object HtmlDirectives {

  def completeTryHtml[A](resultFuture: ⇒ Future[A])(
    implicit
      toHtmlPage: A ⇒ Future[HtmlPage],
      webServiceContext: WebServiceContext,
      ec: ExecutionContext,
      toResponseMarshallable: A ⇒ ToResponseMarshallable): Route
  =
    htmlPreferred(webServiceContext) {
      complete(resultFuture flatMap toHtmlPage)
    } ~
      complete(resultFuture map toResponseMarshallable)

  def htmlPreferred(webServiceContext: WebServiceContext): Directive0 =
    mapInnerRoute { route ⇒
      if (webServiceContext.htmlEnabled)
        requestInstance { request ⇒
          if (request.header[Accept] exists { o ⇒ isHtmlPreferred(o.mediaRanges) })
            handleRejections(RejectionHandler.Default) {
              route
            }
          else
            reject
        }
      else
        reject
    }

  /**
    * Workaround for Spray 1.3.3, which weights the MediaType ordering of the UnMarshaller over the (higher) weight of more specific MediaRange.
    * <p>
    * <a href="https://tools.ietf.org/html/rfc7231#section-5.3.2">https://tools.ietf.org/html/rfc7231#section-5.3.2</a>.
    */
  private def isHtmlPreferred(mediaRanges: Iterable[MediaRange]): Boolean =
    mediaRanges exists {
      case MediaRange.One(`text/html`, 1.0f) ⇒ true  // Highest priority q < 1 is not respected (and should be unusual for a browser)
      case _ ⇒ false
    }
}
