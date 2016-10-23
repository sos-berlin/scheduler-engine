package com.sos.scheduler.engine.common.sprayutils

import java.nio.charset.StandardCharsets.UTF_8
import scala.util.control.NonFatal
import spray.http.HttpCharsets.`UTF-8`
import spray.http.MediaTypes._
import spray.http.{ContentType, HttpEntity}
import spray.httpx.marshalling.{Marshaller, MarshallingContext}

/**
 * @author Joacim Zschimmer
 */
final case class ContentTypedString(string: String, contentType: ContentType)

object ContentTypedString {

  val HtmlString: (String) ⇒ ContentTypedString =
    ContentTypedString(`text/html` withCharset `UTF-8`)

  def apply(contentType: ContentType)(string: String) = new ContentTypedString(string, contentType)

  implicit val marshaller: Marshaller[ContentTypedString] =
    new Marshaller[ContentTypedString] {
      def apply(value: ContentTypedString, ctx: MarshallingContext): Unit =
        try {
          val contentTypes = value.contentType :: Nil
          if (ctx.tryAccept(contentTypes).isDefined)
            ctx.marshalTo(HttpEntity(value.contentType, value.string.getBytes(UTF_8)))
          else
            ctx.rejectMarshalling(contentTypes)
        } catch {
          case NonFatal(e) ⇒ ctx.handleError(e)
        }
    }
}
