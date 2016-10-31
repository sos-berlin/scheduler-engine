package com.sos.scheduler.engine.client.agent

import akka.actor.ActorSystem
import com.sos.scheduler.engine.agent.client.AgentClient
import com.sos.scheduler.engine.base.generic.SecretString
import com.sos.scheduler.engine.base.utils.ScalaUtils.SwitchStatement
import com.sos.scheduler.engine.common.auth.{UserAndPassword, UserId}
import com.sos.scheduler.engine.common.configutils.Configs.ConvertibleConfig
import com.sos.scheduler.engine.common.scalautil.ConcurrentMemoizer
import com.sos.scheduler.engine.common.soslicense.LicenseKeyString
import com.sos.scheduler.engine.common.sprayutils.https.{Https, KeystoreReference}
import com.sos.scheduler.engine.data.agent.AgentAddress
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
  config: Config)
extends (AgentAddress ⇒ AgentClient) {

  private val userAndPasswordOption: Option[UserAndPassword] =
    for (password ← config.optionAs[String]("jobscheduler.master.credentials.password") map SecretString.apply)
       yield UserAndPassword(UserId(conf.schedulerId.string), password)
  private val memoizingAcceptTlsCertificateFor = ConcurrentMemoizer.strict {
    (keystore: KeystoreReference, host: NonEmptyHost, port: Int) ⇒ Https.acceptTlsCertificateFor(keystore, host.address, port)
  }

  def apply(agentUri: AgentAddress): AgentClient = apply(Uri(agentUri.string))

  def apply(agentUri: Uri): AgentClient = {
    (conf.keystoreReferenceOption, agentUri) switch {
      case (Some(keystoreRef), Uri("https", Uri.Authority(host: Uri.NonEmptyHost, port, _), _, _, _)) ⇒
        memoizingAcceptTlsCertificateFor(keystoreRef, host, port)
    }
    new AgentClient.Standard(
      agentUri = agentUri,
      licenseKeys = SchedulerAgentClientFactory.this.licenseKeys,
      userAndPasswordOption = SchedulerAgentClientFactory.this.userAndPasswordOption)
  }
}
