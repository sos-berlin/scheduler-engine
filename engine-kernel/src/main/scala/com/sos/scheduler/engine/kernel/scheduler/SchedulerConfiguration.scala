package com.sos.scheduler.engine.kernel.scheduler

import com.sos.jobscheduler.base.generic.SecretString
import com.sos.jobscheduler.base.utils.ScalaUtils.someUnless
import com.sos.jobscheduler.base.utils.ScalazStyle.OptionRichBoolean
import com.sos.jobscheduler.common.configutils.Configs
import com.sos.jobscheduler.common.scalautil.Collections.emptyToNone
import com.sos.jobscheduler.common.scalautil.FileUtils.implicits._
import com.sos.jobscheduler.common.sprayutils.https.KeystoreReference
import com.sos.jobscheduler.common.utils.JavaResource
import com.sos.jobscheduler.data.scheduler.SchedulerId
import com.sos.scheduler.engine.data.scheduler.{ClusterMemberId, SupervisorUri}
import com.sos.scheduler.engine.kernel.cppproxy.SpoolerC
import com.typesafe.config.Config
import java.io.File
import java.nio.file.Files._
import java.nio.file.{Files, Path, Paths}

trait SchedulerConfiguration {

  private[kernel] def initialize(): Unit

  def clusterMemberId: ClusterMemberId

  def mainConfigurationDirectory: File

  def mainConfigurationFile: File

  def localConfigurationDirectory: File

  def logDirectory: File

  def schedulerId: SchedulerId

  def supervisorUriOption: Option[SupervisorUri]

  def httpPortOption: Option[String]

  def httpsPortOption: Option[String]

  def tcpPort: Int

  def udpPort: Option[Int]

  def existingHtmlDirOption: Option[Path] =
    htmlDirOption match {
      case Some(dir) if Files.exists(dir) ⇒ Some(dir)
      case _ ⇒ None
    }

  def htmlDirOption: Option[Path]

  lazy val keystoreReferenceOption = {
    val file = mainConfigurationDirectory / "agent-https.jks"
    exists(file) option KeystoreReference(
      file.toUri.toURL,
      Some(SecretString("jobscheduler")),
      Some(SecretString("jobscheduler")))
  }

  def jobHistoryTableName: String

  def tasksTableName: String

  def orderHistoryTableName: String

  def orderStepHistoryTableName: String

  def ordersTableName: String

  def variablesTableName: String

  def clustersTableName: String
}

object SchedulerConfiguration {
  lazy val DefaultConfig: Config = Configs.loadResource(JavaResource("com/sos/scheduler/engine/kernel/configuration/master.conf"))

  private[kernel] final class Injectable(spoolerC: SpoolerC) extends SchedulerConfiguration {
    private lazy val settingsC = spoolerC.settings

    def initialize(): Unit = {
      clusterMemberId
      mainConfigurationDirectory
      mainConfigurationFile
      localConfigurationDirectory
      logDirectory
      schedulerId
      supervisorUriOption
      httpPortOption
      httpsPortOption
      tcpPort
      udpPort
      htmlDirOption
      jobHistoryTableName
      tasksTableName
      orderHistoryTableName
      orderStepHistoryTableName
      ordersTableName
      variablesTableName
      clustersTableName
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

    lazy val supervisorUriOption = emptyToNone(spoolerC.supervisor_uri) map SupervisorUri.apply

    lazy val httpPortOption: Option[String] = settingsC._http_port match {
      case "" | "0" ⇒ None
      case o ⇒ Some(o)
    }

    lazy val httpsPortOption: Option[String] = settingsC._https_port match {
        case "" | "0" ⇒ None
        case o ⇒ Some(o)
      }

    lazy val tcpPort: Int =
      spoolerC.tcp_port

    lazy val udpPort: Option[Int] = someUnless(spoolerC.udp_port, 0)

    lazy val htmlDirOption = emptyToNone(settingsC._html_dir) map { o ⇒ Paths.get(o) }

    lazy val jobHistoryTableName       = settingsC._job_history_tablename
    lazy val tasksTableName            = settingsC._tasks_tablename
    lazy val orderHistoryTableName     = settingsC._order_history_tablename
    lazy val orderStepHistoryTableName = settingsC._order_step_history_tablename
    lazy val ordersTableName           = settingsC._orders_tablename
    lazy val variablesTableName        = settingsC._variables_tablename
    lazy val clustersTableName         = settingsC._clusters_tablename
  }
}
