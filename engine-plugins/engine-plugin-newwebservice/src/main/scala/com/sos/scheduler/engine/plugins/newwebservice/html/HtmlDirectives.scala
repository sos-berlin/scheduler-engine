package com.sos.scheduler.engine.plugins.newwebservice.html

import com.sos.scheduler.engine.plugins.newwebservice.html.HtmlDirectives._
import scala.concurrent.{ExecutionContext, Future}
import scala.language.implicitConversions
import spray.http.HttpHeaders.Accept
import spray.http.MediaRange
import spray.http.MediaTypes.`text/html`
import spray.httpx.marshalling.ToResponseMarshallable
import spray.routing.Directives._
import spray.routing.{RejectionHandler, _}

/**
  * @author Joacim Zschimmer
  */
trait HtmlDirectives {

  protected implicit def webServiceContext: WebServiceContext

  def completeAsHtmlPageOrOther[A](future: ⇒ Future[A])(
    implicit
      toHtmlPage: A ⇒ Future[HtmlPage],
      webServiceContext: WebServiceContext,
      ec: ExecutionContext,
      toResponseMarshallable: A ⇒ ToResponseMarshallable): Route
  =
    htmlPreferred {
      complete(future flatMap toHtmlPage)
    } ~
      complete(future map toResponseMarshallable)

  def htmlPreferred: Directive0 =
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
}

object HtmlDirectives {
  /**
    * Workaround for Spray 1.3.3, which priorities the MediaType ordering of the UnMarshaller over higher weight of more specific MediaRange.
    * <p>
    * <a href="https://tools.ietf.org/html/rfc7231#section-5.3.2">https://tools.ietf.org/html/rfc7231#section-5.3.2</a>.
    */
  private def isHtmlPreferred(mediaRanges: Iterable[MediaRange]): Boolean =
    mediaRanges exists {
      case MediaRange.One(`text/html`, 1.0f) ⇒ true  // Highest priority q < 1 is not respected (and should be unusual for a browser)
      case _ ⇒ false
    }
}
