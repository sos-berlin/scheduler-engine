package com.sos.scheduler.engine.plugins.newwebservice.html

import java.io.StringWriter
import scala.concurrent.{ExecutionContext, Future}
import scala.language.implicitConversions
import spray.http.HttpCharsets.`UTF-8`
import spray.http.HttpHeaders.Accept
import spray.http.MediaTypes.`text/html`
import spray.http.{HttpEntity, MediaRange}
import spray.httpx.marshalling.{Marshaller, ToResponseMarshallable}
import spray.routing.Directives._
import spray.routing._

/**
  * @author Joacim Zschimmer
  */
trait HtmlPage {
  def node: xml.Node
}

object HtmlPage {

  implicit val marshaller = Marshaller.of[HtmlPage](`text/html`) { (htmlPage, contentType, ctx) ⇒
    val writer = new StringWriter
    xml.XML.write(writer, htmlPage.node,
      enc = (contentType.definedCharset getOrElse `UTF-8`).toString,
      xmlDecl = false, doctype = xml.dtd.DocType("html"))
    ctx.marshalTo(HttpEntity(contentType, writer.toString))
  }

  def completeAsHtmlPageOrOther[A](future: ⇒ Future[A])
    (implicit toHtmlPage: A ⇒ Future[HtmlPage], ec: ExecutionContext, toResponseMarshallable: A ⇒ ToResponseMarshallable): Route
  =
    htmlPreferred {
      complete(future flatMap toHtmlPage)
    } ~
      complete(future map toResponseMarshallable)

  def htmlPreferred: Directive0 =
    mapInnerRoute { route ⇒
      requestInstance { request ⇒
        if (request.header[Accept] exists { o ⇒ isHtmlPreferred(o.mediaRanges) })
          handleRejections(RejectionHandler.Default) {
            route
          }
        else
          reject
      }
    }

  /**
    * Workaround for Spray 1.3.3, which priorities the MediaType ordering of the UnMarshaller over higher weight of more specific MediaRange.
    * <p>
    * <a href="https://tools.ietf.org/html/rfc7231#section-5.3.2">https://tools.ietf.org/html/rfc7231#section-5.3.2</a>.
    */
  private def isHtmlPreferred(mediaRanges: Iterable[MediaRange]): Boolean =
    mediaRanges exists {
      case MediaRange.One(`text/html`, 1.0f) ⇒ true  // Highest priority q < 1 is not respected (and shoud be unusual for a browser)
      case _ ⇒ false
    }
}
