package com.sos.scheduler.engine.kernel.scheduler

import java.io.File

import com.sos.scheduler.engine.data.scheduler.{ClusterMemberId, SchedulerId}
import com.sos.scheduler.engine.kernel.cppproxy.SpoolerC
import com.sos.scheduler.engine.kernel.settings.CppSettingName
import com.sos.scheduler.engine.kernel.settings.CppSettingName.htmlDir
import javax.inject.{Inject, Singleton}

@Singleton
final class SchedulerConfiguration @Inject private(spoolerC: SpoolerC) {

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

  def tcpPort: Int =
    spoolerC.tcp_port

  def udpPort: Option[Int] =
    spoolerC.udp_port match {
      case 0 ⇒ None
      case n => Some(n)
    }

  def webDirectoryOption: Option[File] =
    setting(htmlDir) match {
      case "" => None
      case o => Some(new File(o))
    }

  private def setting(name: CppSettingName): String =
    spoolerC.setting(name.getNumber)
}
