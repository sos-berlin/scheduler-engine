package com.sos.scheduler.engine.client.agent

import akka.actor.ActorSystem
import com.sos.scheduler.engine.agent.client.AgentClient
import com.sos.scheduler.engine.base.generic.SecretString
import com.sos.scheduler.engine.base.utils.ScalazStyle._
import com.sos.scheduler.engine.client.agent.SchedulerAgentClientFactory._
import com.sos.scheduler.engine.common.auth.{UserAndPassword, UserId}
import com.sos.scheduler.engine.common.configutils.Configs.ConvertibleConfig
import com.sos.scheduler.engine.common.scalautil.Logger
import com.sos.scheduler.engine.common.soslicense.LicenseKeyString
import com.sos.scheduler.engine.common.sprayutils.https.{Https, KeystoreReference}
import com.sos.scheduler.engine.data.agent.AgentAddress
import com.sos.scheduler.engine.data.scheduler.SchedulerId
import com.sos.scheduler.engine.kernel.scheduler.SchedulerConfiguration
import com.typesafe.config.Config
import java.io.File
import java.nio.file.Files.getLastModifiedTime
import java.nio.file.attribute.FileTime
import javax.inject.{Inject, Singleton}
import scala.collection.immutable
import spray.can.Http.HostConnectorSetup
import spray.http.Uri
import spray.http.Uri.NonEmptyHost

/**
  * @author Joacim Zschimmer
  */
@Singleton
final class SchedulerAgentClientFactory private[client](
  actorSystem: ActorSystem,
  schedulerId: SchedulerId,
  keystoreRefOption: Option[KeystoreReference],
  licenseKeys: immutable.Iterable[LicenseKeyString],
  config: Config)
extends (AgentAddress ⇒ AgentClient)
{
  private implicit def implicitActorSystem = actorSystem

  @Inject private[client] def this(
    actorSystem: ActorSystem,
    conf: SchedulerConfiguration,
    licenseKeys: immutable.Iterable[LicenseKeyString],
    config: Config)
  = this(actorSystem, conf.schedulerId, conf.keystoreReferenceOption, licenseKeys, config)

  private val cache = new java.util.concurrent.ConcurrentHashMap[Key, Entry]

  private val userAndPasswordOption: Option[UserAndPassword] =
    for (password ← config.optionAs[String]("jobscheduler.master.credentials.password") map SecretString.apply)
       yield UserAndPassword(UserId(schedulerId.string), password)

  def apply(agentUri: AgentAddress): AgentClient =
    apply(Uri(agentUri.string))

  def apply(agentUri: Uri): AgentClient = {
    val hostConnectorSetupOption = (keystoreRefOption, agentUri) match {
      case (Some(keystoreRef), Uri("https", Uri.Authority(host: Uri.NonEmptyHost, port, _), _, _, _)) ⇒
        val lastModifiedAt = (keystoreRef.url.getProtocol == "file") option
          getLastModifiedTime(new File(keystoreRef.url.toURI).toPath)
        val key = Key(keystoreRef, host, port)
        def toEntry() = Entry(Https.toHostConnectorSetup(keystoreRef, host, port), lastModifiedAt)
        var entry = cache.computeIfAbsent(key,
          new java.util.function.Function[Key, Entry] {
            def apply(key: Key) = toEntry()
          })
        if (entry.lastModifiedAt != lastModifiedAt) {
          logger.debug(s"Reloading modified ${keystoreRef.url}")
          entry = toEntry()
          cache.put(key, entry)
        }
        Some(entry.hostConnectorSetup)

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

object SchedulerAgentClientFactory
{
  private val logger = Logger(getClass)
  private case class Key(keystoreRef: KeystoreReference, host: NonEmptyHost, port: Int)
  private case class Entry(hostConnectorSetup: HostConnectorSetup, lastModifiedAt: Option[FileTime])
}
