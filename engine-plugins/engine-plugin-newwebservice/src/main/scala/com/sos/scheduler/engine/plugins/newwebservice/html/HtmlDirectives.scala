package com.sos.scheduler.engine.plugins.newwebservice.html

import com.sos.scheduler.engine.plugins.newwebservice.html.HtmlPage._
import scala.concurrent.{ExecutionContext, Future}
import scala.language.implicitConversions
import spray.http.HttpHeaders.Accept
import spray.http.MediaTypes.`text/html`
import spray.http.StatusCodes._
import spray.http.{HttpMethods, MediaRange, Uri}
import spray.httpx.marshalling.ToResponseMarshallable
import spray.routing.Directives._
import spray.routing._

/**
  * @author Joacim Zschimmer
  */
object HtmlDirectives {

  /**
    * If HTML is requested, path ends with slash and request has no query, then redirect to path without slash, in case of typo.
    */
  def pathEndElseRedirect(webServiceContext: WebServiceContext): Directive0 =
    mapInnerRoute { route ⇒
      pathEnd {
        route
      } ~
      pathSingleSlash {
        htmlPreferred(webServiceContext) {
          requestInstance { request ⇒
            if (request.uri.query == Uri.Query.Empty) {
              val withoutSlash = request.uri.copy(
                scheme = "",
                authority = Uri.Authority.Empty,
                path = Uri.Path(request.uri.path.toString stripSuffix "/"))
              redirect(withoutSlash, TemporaryRedirect)
            } else
              reject
          }
        }
      }
    }

  /**
    * If HTML is requested, trailing slash is missing and request has no query, then redirect to trailing slash, in case of typo.
    */
  def eatSlash(webServiceContext: WebServiceContext): Directive0 =
    mapInnerRoute { route ⇒
      pathEnd {
        htmlPreferred(webServiceContext) {  // The browser user may type "api/"
          requestInstance { request ⇒
            if (request.uri.query == Uri.Query.Empty) {
              val withSlash = request.uri.copy(
                scheme = "",
                authority = Uri.Authority.Empty,
                path = Uri.Path(request.uri.path.toString + "/"))
              redirect(withSlash, TemporaryRedirect)
            } else
              reject
          }
        }
      } ~
        route
    }

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
          if (request.method == HttpMethods.GET &&
             (request.header[Accept] exists { o ⇒ isHtmlPreferred(o.mediaRanges) }))
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
