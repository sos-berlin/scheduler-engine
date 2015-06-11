package com.sos.scheduler.engine.plugins.jetty.test

import JettyPluginJerseyTester._
import com.google.inject.Injector
import com.sos.scheduler.engine.common.scalautil.ScalaUtils.implicitClass
import com.sos.scheduler.engine.common.scalautil.xmls.SafeXML
import com.sos.scheduler.engine.common.scalautil.{Logger, HasCloser}
import com.sos.scheduler.engine.common.time.ScalaTime._
import com.sos.scheduler.engine.plugins.jetty.test.JettyPluginTests._
import java.net.URI
import javax.ws.rs.core.MediaType
import scala.reflect.ClassTag
import java.io.Reader
import scala.util.Try

/**
 * @author Joacim Zschimmer
 */
trait JettyPluginJerseyTester extends HasCloser {

  final val host = "127.0.0.1"
  private lazy val webClient = newAuthentifyingClient(webTimeout)
  protected lazy val webTimeout = 60.s
  lazy final val port: Int = jettyPortNumber(injector)
  lazy val webResource = webClient.resource(s"http://$host:$port/")

  onClose {
    webClient.destroy()
  }

  def get[A : ClassTag](uriString: String, Accept: Iterable[MediaType] = Nil): A = {
    val uri = new URI(uriString)
    val r = webResource.uri(uri).accept(Accept.toArray: _*)
    val result = Try {
      if (implicitClass[A] eq classOf[xml.Elem])
        (SafeXML.load(r.get(classOf[Reader])): xml.Elem).asInstanceOf[A]
      else
        r.get(implicitClass[A])
    }
    logger.debug(s"HTTP GET $uri => $result")
    result.get
  }

//  def post[A : ClassTag](uri: String, content: AnyRef, `Content-Type`: MediaType = null, Accept: Iterable[MediaType] = Nil): A = {
//    val b = webResource.uri(new URI(uri)).getRequestBuilder
//    if (`Content-Type` != null) b.`type`(`Content-Type`)
//    b.accept(Accept.toArray: _*)
//    b.post(implicitClass[A], content)
//  }

  def injector: Injector
}

object JettyPluginJerseyTester {
  private val logger = Logger(getClass)

  /**
   * Replaces some neither reserved nor unreserved characters, that would be rejected by [[java.net.URI]] - for convenience.
   */
  def normalizeUri(o: String) =
    o.replace("<", "%3C")
     .replace(">", "%3E")
}
