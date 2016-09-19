package com.sos.scheduler.engine.kernel.scheduler

import com.sos.scheduler.engine.base.generic.SecretString
import com.sos.scheduler.engine.base.utils.ScalaUtils.someUnless
import com.sos.scheduler.engine.base.utils.ScalazStyle.OptionRichBoolean
import com.sos.scheduler.engine.common.configutils.Configs
import com.sos.scheduler.engine.common.scalautil.Collections.emptyToNone
import com.sos.scheduler.engine.common.scalautil.FileUtils.implicits._
import com.sos.scheduler.engine.common.sprayutils.https.KeystoreReference
import com.sos.scheduler.engine.common.utils.JavaResource
import com.sos.scheduler.engine.data.scheduler.{ClusterMemberId, SchedulerId}
import com.sos.scheduler.engine.kernel.cppproxy.SpoolerC
import com.typesafe.config.Config
import java.io.File
import java.net.{URI, URL}
import java.nio.file.Files._
import java.nio.file.{Path, Paths}

trait SchedulerConfiguration {

  private[kernel] def initialize(): Unit

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

  def htmlDirOption: Option[Path]

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

  private[kernel] final class Injectable (spoolerC: SpoolerC) extends SchedulerConfiguration {
    private lazy val settingsC = spoolerC.settings

    def initialize(): Unit = {
      clusterMemberId
      mainConfigurationDirectory
      mainConfigurationFile
      localConfigurationDirectory
      logDirectory
      schedulerId
      httpPortOption
      tcpPort
      udpPort
      webDirectoryUrlOption
      htmlDirOption
    }

    lazy val clusterMemberId: ClusterMemberId =
      ClusterMemberId(spoolerC.cluster_member_id)

    lazy val mainConfigurationDirectory: File =
      Option(mainConfigurationFile.getParentFile) getOrElse new File(".")

    lazy val mainConfigurationFile: File =
      new File(spoolerC.configuration_file_path)

    /** Das Verzeichnis der Konfigurationsdatei scheduler.xml, Normalerweise "config" */
    lazy val localConfigurationDirectory: File =
      new File(spoolerC.local_configuration_directory)

    lazy val logDirectory: File =
      spoolerC.log_directory match {
        case null | "" | "*stderr" ⇒ throw new SchedulerException("Scheduler runs without a log directory")
        case o ⇒ new File(o)
      }

    lazy val schedulerId: SchedulerId =
      SchedulerId(spoolerC.id)

    lazy val httpPortOption: Option[Int] = someUnless(settingsC._http_port, 0)

    lazy val tcpPort: Int =
      spoolerC.tcp_port

    lazy val udpPort: Option[Int] = someUnless(spoolerC.udp_port, 0)

    lazy val webDirectoryUrlOption: Option[URL] =
      settingsC._web_directory match {
        case "" ⇒ None
        case o ⇒ Some(new URI(o).toURL)
      }

    lazy val htmlDirOption = emptyToNone(settingsC._html_dir) map { o ⇒ Paths.get(o) }
  }
}
