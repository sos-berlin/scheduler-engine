package com.sos.scheduler.engine.kernel.scheduler

import com.sos.scheduler.engine.base.generic.SecretString
import com.sos.scheduler.engine.common.configutils.Configs
import com.sos.scheduler.engine.common.scalautil.FileUtils.implicits._
import com.sos.scheduler.engine.common.scalautil.ScalaUtils.someUnless
import com.sos.scheduler.engine.common.scalautil.ScalazStyle.OptionRichBoolean
import com.sos.scheduler.engine.common.sprayutils.https.KeystoreReference
import com.sos.scheduler.engine.common.utils.JavaResource
import com.sos.scheduler.engine.data.scheduler.{ClusterMemberId, SchedulerId}
import com.sos.scheduler.engine.kernel.cppproxy.SpoolerC
import com.typesafe.config.Config
import java.io.File
import java.net.{URI, URL}
import java.nio.file.Files._
import javax.inject.{Inject, Provider}

trait SchedulerConfiguration {

  def clusterMemberId: ClusterMemberId

  def mainConfigurationDirectory: File

  def mainConfigurationFile: File

  def localConfigurationDirectory: File

  def logDirectory: File

  def schedulerId: SchedulerId

  def httpPortOption: Option[Int]

  def tcpPort: Int

  def udpPort: Option[Int]

  def webDirectoryUrlOption: Option[URL]

  lazy val keystoreReferenceOption = {
    val file = mainConfigurationDirectory / "agent-https.jks"
    exists(file) option KeystoreReference(
      file.toUri.toURL,
      Some(SecretString("jobscheduler")),
      Some(SecretString("jobscheduler")))
  }
}

object SchedulerConfiguration {
  lazy val DefaultConfig: Config = Configs.loadResource(JavaResource("com/sos/scheduler/engine/kernel/configuration/defaults.conf"))

  final class InjectProvider @Inject private(spoolerC: SpoolerC) extends Provider[SchedulerConfiguration] {
    private lazy val settingsC = spoolerC.settings

    private lazy val conf = new SchedulerConfiguration {
      def clusterMemberId: ClusterMemberId =
        ClusterMemberId(spoolerC.cluster_member_id)

      def mainConfigurationDirectory: File =
        Option(mainConfigurationFile.getParentFile) getOrElse new File(".")

      def mainConfigurationFile: File =
        new File(spoolerC.configuration_file_path)

      /** Das Verzeichnis der Konfigurationsdatei scheduler.xml, Normalerweise "config" */
      def localConfigurationDirectory: File =
        new File(spoolerC.local_configuration_directory)

      def logDirectory: File =
        spoolerC.log_directory match {
          case null | "" | "*stderr" ⇒ throw new SchedulerException("Scheduler runs without a log directory")
          case o ⇒ new File(o)
        }

      def schedulerId: SchedulerId =
        SchedulerId(spoolerC.id)

      lazy val httpPortOption: Option[Int] = someUnless(settingsC._http_port, 0)

      lazy val tcpPort: Int =
        spoolerC.tcp_port

      def udpPort: Option[Int] = someUnless(spoolerC.udp_port, 0)

      lazy val webDirectoryUrlOption: Option[URL] =
        settingsC._web_directory match {
          case "" ⇒ None
          case o ⇒ Some(new URI(o).toURL)
        }
    }

    def get() = conf
  }
}
