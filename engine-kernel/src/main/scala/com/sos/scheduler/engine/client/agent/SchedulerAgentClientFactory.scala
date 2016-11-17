package com.sos.scheduler.engine.client.agent

import akka.actor.ActorSystem
import com.sos.scheduler.engine.agent.client.AgentClient
import com.sos.scheduler.engine.base.generic.SecretString
import com.sos.scheduler.engine.common.auth.UserAndPassword
import com.sos.scheduler.engine.common.configutils.Configs.ConvertibleConfig
import com.sos.scheduler.engine.common.scalautil.ConcurrentMemoizer
import com.sos.scheduler.engine.common.soslicense.LicenseKeyString
import com.sos.scheduler.engine.common.sprayutils.https.{Https, KeystoreReference}
import com.sos.scheduler.engine.kernel.scheduler.SchedulerConfiguration
import com.typesafe.config.Config
import javax.inject.{Inject, Singleton}
import scala.collection.immutable
import spray.http.Uri
import spray.http.Uri.NonEmptyHost

/**
  * @author Joacim Zschimmer
  */
@Singleton
final class SchedulerAgentClientFactory @Inject private[client](
  implicit private val actorSystem: ActorSystem,
  conf: SchedulerConfiguration,
  licenseKeys: immutable.Iterable[LicenseKeyString],
  config: Config) {

  private val memoizingHostConnectorSetupFor = ConcurrentMemoizer.strict {
    (keystore: KeystoreReference, host: NonEmptyHost, port: Int) ⇒
      Https.toHostConnectorSetup(keystore, host, port)
  }

  def apply(agentUri: String): AgentClient = {
    val hostConnectorSetupOption_ = (conf.keystoreReferenceOption, Uri(agentUri)) match {
      case (Some(keystoreRef), Uri("https", Uri.Authority(host: Uri.NonEmptyHost, port, _), _, _, _)) ⇒
        Some(memoizingHostConnectorSetupFor(keystoreRef, host, port))
      case _ ⇒
        None
    }
    val agentUri_ = agentUri
    new AgentClient {
      val agentUri = agentUri_
      def licenseKeys = SchedulerAgentClientFactory.this.licenseKeys
      val actorRefFactory = SchedulerAgentClientFactory.this.actorSystem
      val hostConnectorSetupOption = hostConnectorSetupOption_
      val userAndPasswordOption = config.optionAs[String]("jobscheduler.master.credentials.password") map
        SecretString map { UserAndPassword(conf.schedulerId.string, _) }
    }
  }
}
