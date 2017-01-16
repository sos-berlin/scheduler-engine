package com.sos.scheduler.engine.plugins.newwebservice

import akka.actor.ActorSystem
import com.google.inject.{AbstractModule, Injector, Provides}
import com.sos.scheduler.engine.common.internet.IP._
import com.sos.scheduler.engine.common.scalautil.Closers.implicits.RichClosersCloser
import com.sos.scheduler.engine.common.scalautil.FileUtils.implicits._
import com.sos.scheduler.engine.common.scalautil.Futures.implicits.SuccessFuture
import com.sos.scheduler.engine.common.scalautil.SetOnce
import com.sos.scheduler.engine.common.sprayutils.WebServerBinding
import com.sos.scheduler.engine.common.sprayutils.https.KeystoreReference
import com.sos.scheduler.engine.common.sprayutils.web.auth.{CSRF, GateKeeper}
import com.sos.scheduler.engine.common.time.ScalaTime._
import com.sos.scheduler.engine.common.xml.CppXmlUtils.toXml
import com.sos.scheduler.engine.kernel.Scheduler
import com.sos.scheduler.engine.kernel.plugin.{Plugin, Plugins, UseGuiceModule}
import com.sos.scheduler.engine.kernel.scheduler.SchedulerConfiguration
import com.sos.scheduler.engine.plugins.newwebservice.configuration.NewWebServicePluginConfiguration
import com.typesafe.config.Config
import javax.inject.{Inject, Named, Singleton}
import org.w3c.dom.Element
import scala.concurrent.ExecutionContext

/**
 * @author Joacim Zschimmer
 */
@UseGuiceModule(classOf[NewWebServiceModule])
final class NewWebServicePlugin @Inject private(
  @Named(Plugins.configurationXMLName) pluginElement: Element,
  schedulerConfiguration: SchedulerConfiguration,
  gateKeeperConfiguration: GateKeeper.Configuration,
  csrf: CSRF,
  config: Config,
  schedulerInjector: Injector)
  (implicit
    actorSystem: ActorSystem,
    executionContext: ExecutionContext)
extends Plugin {

  private val conf = NewWebServiceConfigurationParser.parseString(toXml(pluginElement))
  private val myInjector = schedulerInjector.createChildInjector(new AbstractModule {
    def configure() = {}

    @Provides @Singleton
    def newWebServicePluginConfiguration(): NewWebServicePluginConfiguration = conf
  })
  private val uriOnce = new SetOnce[String](name = "NewWebServicePlugin.uri")

  override def onPrepare() = runWithSprayWebServer()

  override def state = {
    case Scheduler.SosSpoolerUriName ⇒ uriOnce()
  }

  private def runWithSprayWebServer(): Unit = {
    val bindings = (httpBinding ++ httpsBinding).toList
    if (bindings.nonEmpty) {
      val webServer = new EngineWebServer(bindings, gateKeeperConfiguration, csrf, myInjector)
      closer.registerAutoCloseable(webServer)
      webServer.start() await 60.s
      uriOnce := webServer.localUri.toString
    }
  }

  private def httpBinding = for (o ← schedulerConfiguration.httpPortOption) yield
    WebServerBinding.Http(StringToServerInetSocketAddress(o))

  private def httpsBinding = for (o ← schedulerConfiguration.httpsPortOption) yield
    WebServerBinding.Https(StringToServerInetSocketAddress(o), newKeyStoreReference())

  private def newKeyStoreReference() = KeystoreReference.fromSubConfig(
    config.getConfig("jobscheduler.master.webserver.https.keystore"),
    configDirectory = schedulerConfiguration.mainConfigurationDirectory)
}
