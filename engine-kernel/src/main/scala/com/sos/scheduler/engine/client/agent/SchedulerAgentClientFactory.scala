package com.sos.scheduler.engine.client.agent

import akka.actor.ActorSystem
import com.sos.jobscheduler.agent.client.AgentClient
import com.sos.jobscheduler.base.generic.SecretString
import com.sos.jobscheduler.common.auth.{UserAndPassword, UserId}
import com.sos.jobscheduler.common.configutils.Configs.ConvertibleConfig
import com.sos.jobscheduler.common.scalautil.ConcurrentMemoizer
import com.sos.jobscheduler.common.soslicense.LicenseKeyString
import com.sos.jobscheduler.common.sprayutils.https.{Https, KeystoreReference}
import com.sos.jobscheduler.data.agent.AgentAddress
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
  private val memoizingHostConnectorSetupFor = ConcurrentMemoizer.strict {
    (keystore: KeystoreReference, host: NonEmptyHost, port: Int) ⇒
      Https.toHostConnectorSetup(keystore, host, port)
  }

  def apply(agentUri: AgentAddress): AgentClient =
    apply(Uri(agentUri.string))

  def apply(agentUri: Uri): AgentClient = {
    val hostConnectorSetupOption = (conf.keystoreReferenceOption, agentUri) match {
      case (Some(keystoreRef), Uri("https", Uri.Authority(host: Uri.NonEmptyHost, port, _), _, _, _)) ⇒
        Some(memoizingHostConnectorSetupFor(keystoreRef, host, port))
      case _ ⇒
        None
    }
    AgentClient(
      agentUri,
      SchedulerAgentClientFactory.this.licenseKeys,
      hostConnectorSetupOption,
      SchedulerAgentClientFactory.this.userAndPasswordOption)
  }
}
