package com.sos.scheduler.engine.plugins.newwebservice.html

import com.sos.scheduler.engine.plugins.newwebservice.common.SprayUtils._
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
  def pathEndRedirectToSlash(webServiceContext: WebServiceContext): Route =
    pathEnd {
      redirectEmptyQueryBy(webServiceContext, path ⇒ Uri.Path(path.toString + "/"))
    }

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
          get {
            requestInstance { request ⇒
              passIf(request.uri.query == Uri.Query.Empty) {
                val withoutSlash = request.uri.copy(
                  scheme = "",
                  authority = Uri.Authority.Empty,
                  path = Uri.Path(request.uri.path.toString stripSuffix "/"))
                redirect(withoutSlash, TemporaryRedirect)
              }
            }
          }
        }
      }
    }

  /**
    * If HTML is requested, trailing slash is missing and request has no query, then redirect to trailing slash, in case of typo.
    */
  def testSlash(webServiceContext: WebServiceContext): Directive0 =
    mapInnerRoute { route ⇒
      redirectToSlash(webServiceContext) ~
      unmatchedPath {
        case path: Uri.Path.Slash ⇒ route
        case _ ⇒ reject
      }
    }

  /**
    * If HTML is requested, trailing slash is missing and request has no query, then redirect to trailing slash, in case of typo.
    */
  def redirectToSlash(webServiceContext: WebServiceContext): Route =
    pathEnd {
      htmlPreferred(webServiceContext) {  // The browser user may type "api/"
        requestInstance { request ⇒
          passIf(request.uri.query == Uri.Query.Empty) {
            val withSlash = request.uri.copy(
              scheme = "",
              authority = Uri.Authority.Empty,
              path = Uri.Path(request.uri.path.toString + "/"))
            redirect(withSlash, TemporaryRedirect)
          }
        }
      }
    }

  /**
    * If HTML is requested and request has no query, then redirect according to `changePath`, in case of user typo.
    */
  def redirectEmptyQueryBy(webServiceContext: WebServiceContext, changePath: Uri.Path ⇒ Uri.Path): Route =
    htmlPreferred(webServiceContext) {
      get {
        requestInstance { request ⇒
          if (request.uri.query == Uri.Query.Empty) {
            redirect(
              request.uri.copy(
                scheme = "",
                authority = Uri.Authority.Empty,
                path = changePath(request.uri.path)),
              TemporaryRedirect)
          } else
            reject
        }
      }
    }

  def completeTryHtml[A](resultFuture: ⇒ Future[A])(
    implicit
      toHtmlPage: A ⇒ Future[HtmlPage],
      webServiceContext: WebServiceContext,
      executionContext: ExecutionContext,
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
