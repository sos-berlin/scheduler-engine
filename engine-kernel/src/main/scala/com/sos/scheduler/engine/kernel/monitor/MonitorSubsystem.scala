package com.sos.scheduler.engine.kernel.monitor

import com.google.inject.Injector
import com.sos.scheduler.engine.common.guice.GuiceImplicits.RichInjector
import com.sos.scheduler.engine.cplusplus.runtime.{Sister, SisterType}
import com.sos.scheduler.engine.data.filebased.FileBasedType
import com.sos.scheduler.engine.data.monitor.MonitorPath
import com.sos.scheduler.engine.kernel.async.SchedulerThreadCallQueue
import com.sos.scheduler.engine.kernel.cppproxy.{MonitorC, Monitor_subsystemC}
import com.sos.scheduler.engine.kernel.filebased.FileBasedSubsystem
import com.sos.scheduler.engine.kernel.persistence.hibernate.HibernateTaskStore
import com.sos.scheduler.engine.kernel.scheduler.HasInjector
import javax.inject.Inject
import javax.persistence.EntityManagerFactory

private[kernel] final class MonitorSubsystem @Inject private(
  protected[this] val cppProxy: Monitor_subsystemC,
  implicit val schedulerThreadCallQueue: SchedulerThreadCallQueue,
  protected val injector: Injector)
extends FileBasedSubsystem
with Sister
{
  type ThisSubsystemClient = MonitorSubsystemClient
  type ThisSubsystem = MonitorSubsystem
  type Path = MonitorPath
  type ThisFileBased = MonitorSister
  type ThisFile_basedC = MonitorC

  val companion = MonitorSubsystem

  private[monitor] lazy val entityManagerFactory = injector.instance[EntityManagerFactory]
  private[monitor] lazy val taskStore = injector.instance[HibernateTaskStore]

  override def onCppProxyInvalidated() = {}

  private[kernel] override def overview: MonitorSubsystemOverview = {
    val superOverview = super.overview
    MonitorSubsystemOverview(
      fileBasedType = superOverview.fileBasedType,
      count = superOverview.count,
      fileBasedStateCounts = superOverview.fileBasedStateCounts)
  }
}

object MonitorSubsystem
extends FileBasedSubsystem.AbstractCompanion[MonitorSubsystemClient, MonitorSubsystem, MonitorPath, MonitorSister]
{
  val fileBasedType = FileBasedType.Monitor
  val stringToPath = MonitorPath.apply _

  private[kernel] object Type extends SisterType[MonitorSubsystem, Monitor_subsystemC] {
    def sister(proxy: Monitor_subsystemC, context: Sister) = {
      val injector = context.asInstanceOf[HasInjector].injector
      new MonitorSubsystem(proxy, injector.instance[SchedulerThreadCallQueue], injector)
    }
  }
}


