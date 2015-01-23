package com.sos.scheduler.engine.kernel.scheduler

import com.sos.scheduler.engine.common.scalautil.ScalaUtils.someUnless
import com.sos.scheduler.engine.data.scheduler.{ClusterMemberId, SchedulerId}
import com.sos.scheduler.engine.kernel.cppproxy.SpoolerC
import java.io.File
import java.net.{URI, URL}
import javax.inject.{Inject, Provider}

trait SchedulerConfiguration {

  def clusterMemberId: ClusterMemberId

  def mainConfigurationDirectory: File

  def mainConfigurationFile: File

  /** Das Verzeichnis der Konfigurationsdatei scheduler.xml, Normalerweise "config" */
  def localConfigurationDirectory: File

  def logDirectory: File

  def schedulerId: SchedulerId

  def httpPortOption: Option[Int]

  def tcpPort: Int

  def udpPort: Option[Int]

  def webDirectoryUrlOption: Option[URL]
}

object SchedulerConfiguration {
  class InjectProvider @Inject private(spoolerC: SpoolerC) extends Provider[SchedulerConfiguration] {
    private lazy val settingsC = spoolerC.settings

    def get() = new SchedulerConfiguration {
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
          case null | "" | "*stderr" => throw new SchedulerException("Scheduler runs without a log directory")
          case o => new File(o)
        }

      def schedulerId: SchedulerId =
        SchedulerId(spoolerC.id)

      lazy val httpPortOption: Option[Int] = someUnless(settingsC._http_port, 0)

      lazy val tcpPort: Int =
        spoolerC.tcp_port

      def udpPort: Option[Int] = someUnless(spoolerC.udp_port, 0)

      lazy val webDirectoryUrlOption: Option[URL] =
        settingsC._web_directory match {
          case "" => None
          case o => Some(new URI(o).toURL)
        }
    }
  }
}
