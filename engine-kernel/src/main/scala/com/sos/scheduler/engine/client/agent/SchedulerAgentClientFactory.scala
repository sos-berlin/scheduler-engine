package com.sos.scheduler.engine.client.agent

import akka.actor.ActorSystem
import com.sos.scheduler.engine.agent.client.AgentClient
import com.sos.scheduler.engine.base.generic.SecretString
import com.sos.scheduler.engine.client.agent.SchedulerAgentClientFactory._
import com.sos.scheduler.engine.common.auth.UserAndPassword
import com.sos.scheduler.engine.common.scalautil.AutoClosing.autoClosing
import com.sos.scheduler.engine.common.scalautil.ConcurrentMemoizer
import com.sos.scheduler.engine.common.scalautil.FileUtils.implicits._
import com.sos.scheduler.engine.common.scalautil.ScalazStyle.OptionRichBoolean
import com.sos.scheduler.engine.common.soslicense.LicenseKeyString
import com.sos.scheduler.engine.common.sprayutils.https.Https
import com.sos.scheduler.engine.kernel.scheduler.SchedulerConfiguration
import java.nio.file.Files.exists
import java.nio.file.Path
import javax.inject.{Inject, Singleton}
import scala.collection.immutable
import spray.http.Uri

/**
  * @author Joacim Zschimmer
  */
@Singleton
final class SchedulerAgentClientFactory @Inject private[client](
  implicit private val actorSystem: ActorSystem,
  schedulerConfiguration: SchedulerConfiguration,
  licenseKeys: immutable.Iterable[LicenseKeyString]) {

  private val keystoreReferenceOption = exists(schedulerConfiguration.agentHttpsKeystoreFile) option
    schedulerConfiguration.agentHttpsKeystoreReference

  private val userAndPassword = {
    val file = schedulerConfiguration.passwordFile
    exists(file) option {
      UserAndPassword(schedulerConfiguration.schedulerId.string, readPassword(file))
    }
  }

  private val acceptOwnTlsCertificate: (Uri.NonEmptyHost, Int) ⇒ Unit =
    keystoreReferenceOption match {
      case Some(keystoreRef) ⇒ ConcurrentMemoizer.strict { (host, port) ⇒
        Https.acceptTlsCertificateFor(keystoreRef, host.address, port)
      }
      case None ⇒ (_, _) ⇒ ()
    }

  def apply(agentUri: String): AgentClient = {
    Uri(agentUri) match {
      case Uri("https", Uri.Authority(host: Uri.NonEmptyHost, port, _), _, _, _) ⇒ acceptOwnTlsCertificate(host, port)
      case _ ⇒
    }

    val pAgentUri = agentUri
    new AgentClient {
      val agentUri = pAgentUri
      def licenseKeys = SchedulerAgentClientFactory.this.licenseKeys
      val actorRefFactory = SchedulerAgentClientFactory.this.actorSystem
      override val userAndPasswordOption = SchedulerAgentClientFactory.this.userAndPassword
    }
  }
}

private object SchedulerAgentClientFactory {
  private[agent] def readPassword(file: Path): SecretString = {
    autoClosing(io.Source.fromFile(file)) { source ⇒
      val lines = source.getLines()
      if (!lines.hasNext) throw new IllegalArgumentException(s"File '$file' is empty")
      val password = SecretString(lines.next())
      while (lines.hasNext) {
        val line = lines.next()
        if (line.nonEmpty) throw new IllegalArgumentException(s"File '$file' contains more than one line")
      }
      password
    }
  }
}
