package com.sos.scheduler.engine.kernel.scheduler

import com.sos.scheduler.engine.data.scheduler.{ClusterMemberId, SchedulerId}
import com.sos.scheduler.engine.kernel.cppproxy.SpoolerC
import com.sos.scheduler.engine.kernel.settings.{CppSettingName, CppSettings}
import java.io.File
import javax.inject.{Inject, Singleton}

@Singleton
final class SchedulerConfiguration @Inject private(spoolerC: SpoolerC) {

  private lazy val settingsC = spoolerC.settings

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

  lazy val httpPort: Int =
    settingsC._http_port

  lazy val tcpPort: Int =
    spoolerC.tcp_port

  def udpPort: Option[Int] =
    spoolerC.udp_port match {
      case 0 ⇒ None
      case n => Some(n)
    }

  lazy val webDirectoryOption: Option[File] =
    settingsC._html_dir match {
      case "" => None
      case o => Some(new File(o))
    }
}
