package com.sos.scheduler.engine.plugins.jetty.test

import com.google.inject.{ConfigurationException, Injector}
import com.sos.scheduler.engine.common.scalautil.SideEffect.ImplicitSideEffect
import com.sos.scheduler.engine.common.time.ScalaTime._
import com.sos.scheduler.engine.data.job.JobPath
import com.sos.scheduler.engine.data.jobchain.JobChainPath
import com.sos.scheduler.engine.kernel.plugin.PluginSubsystem
import com.sos.scheduler.engine.plugins.jetty.JettyPlugin
import com.sos.scheduler.engine.plugins.jetty.configuration.ObjectMapperJacksonJsonProvider
import com.sun.jersey.api.client.Client
import com.sun.jersey.api.client.config.{ClientConfig, DefaultClientConfig}
import com.sun.jersey.api.client.filter.{ClientFilter, HTTPBasicAuthFilter}
import java.net.URI
import java.time.Duration
import scala.math.min
import scala.util.control.NonFatal

object JettyPluginTests {

  private val defaultTimeout = 60.s
  val AJobChainPath = JobChainPath("/a")
  val AJobPath = JobPath("/a")
  val UmlautJobPath = JobPath("/test-umlauts-äöüßÄÖÜ")
  val OrderJobPath = JobPath("/order")

  def contextUri(injector: Injector) =
    new URI("http://127.0.0.1:"+ jettyPortNumber(injector))

  def jettyPortNumber(injector: Injector) =
    jettyPlugin(injector).portNumber

  private def jettyPlugin(injector: Injector): JettyPlugin =
    try injector.getInstance(classOf[JettyPlugin])   // Scheitert, wenn über Scheduler und PluginSubsystem gestartet, weil JettyPlugin in einem Child Injector steckt.
    catch {
      case e: ConfigurationException ⇒
        try injector.getInstance(classOf[PluginSubsystem]).pluginByClass(classOf[JettyPlugin])  // Nur, wenn Test mit Scheduler gestartet ist
        catch { case NonFatal(ee) ⇒ e.addSuppressed(ee); throw e }
    }

  def newAuthentifyingClient(timeout: Duration = defaultTimeout, filters: Iterable[ClientFilter] = Iterable()) = {
    val config = new DefaultClientConfig sideEffect { o ⇒
      o.getSingletons.add(ObjectMapperJacksonJsonProvider)
      o.getProperties.put(ClientConfig.PROPERTY_FOLLOW_REDIRECTS, false: java.lang.Boolean)
    }
    Client.create(config) sideEffect { client ⇒
      client.setReadTimeout(min(timeout.toMillis.toInt, Int.MaxValue))
      client.addFilter(new HTTPBasicAuthFilter("testName", "testPassword"))
      for (f <- filters) client.addFilter(f)
    }
  }
}
