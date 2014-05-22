package com.sos.scheduler.engine.plugins.jetty.test

import com.google.inject.Injector
import com.sos.scheduler.engine.common.scalautil.HasCloser
import com.sos.scheduler.engine.common.scalautil.ScalaUtils.implicitClass
import com.sos.scheduler.engine.common.time.ScalaJoda._
import java.net.URI
import javax.ws.rs.core.MediaType
import scala.reflect.ClassTag
import com.sos.scheduler.engine.plugins.jetty.test.JettyPluginTests._

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

  def get[A : ClassTag](uri: String, Accept: Iterable[MediaType] = Nil): A =
    webResource.uri(new URI(uri)).accept(Accept.toArray: _*).get(implicitClass[A])

//  def post[A : ClassTag](uri: String, content: AnyRef, `Content-Type`: MediaType = null, Accept: Iterable[MediaType] = Nil): A = {
//    val b = webResource.uri(new URI(uri)).getRequestBuilder
//    if (`Content-Type` != null) b.`type`(`Content-Type`)
//    b.accept(Accept.toArray: _*)
//    b.post(implicitClass[A], content)
//  }

  def injector: Injector
}
