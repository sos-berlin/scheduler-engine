package com.sos.scheduler.engine.kernel.configuration

import SchedulerModule._
import com.google.inject.Provides
import com.google.inject.Scopes.SINGLETON
import com.sos.scheduler.engine.common.async.StandardCallQueue
import com.sos.scheduler.engine.cplusplus.runtime.DisposableCppProxyRegister
import com.sos.scheduler.engine.data.scheduler.ClusterMemberId
import com.sos.scheduler.engine.data.scheduler.SchedulerClusterMemberKey
import com.sos.scheduler.engine.data.scheduler.SchedulerId
import com.sos.scheduler.engine.eventbus.{EventBus, SchedulerEventBus}
import com.sos.scheduler.engine.kernel.async.SchedulerThreadCallQueue
import com.sos.scheduler.engine.kernel.command.{CommandHandler, HasCommandHandlers, CommandSubsystem}
import com.sos.scheduler.engine.kernel.cppproxy._
import com.sos.scheduler.engine.kernel.database.DatabaseSubsystem
import com.sos.scheduler.engine.kernel.filebased.FileBasedSubsystem
import com.sos.scheduler.engine.kernel.folder.FolderSubsystem
import com.sos.scheduler.engine.kernel.job.JobSubsystem
import com.sos.scheduler.engine.kernel.lock.LockSubsystem
import com.sos.scheduler.engine.kernel.order.{StandingOrderSubsystem, OrderSubsystem}
import com.sos.scheduler.engine.kernel.plugin.PluginSubsystem
import com.sos.scheduler.engine.kernel.processclass.ProcessClassSubsystem
import com.sos.scheduler.engine.kernel.schedule.ScheduleSubsystem
import com.sos.scheduler.engine.kernel.scheduler._
import com.sos.scheduler.engine.kernel.variable.VariableSet
import com.sos.scheduler.engine.main.SchedulerControllerBridge
import java.util.UUID.randomUUID
import javax.inject.Singleton
import javax.persistence.EntityManagerFactory
import scala.collection.JavaConversions._
import com.sos.scheduler.engine.kernel.log.PrefixLog

final class SchedulerModule(cppProxy: SpoolerC, controllerBridge: SchedulerControllerBridge, schedulerThread: Thread)
extends ScalaAbstractModule {

  def configure() {
    bindInstance(cppProxy)
    bindInstance(controllerBridge)
    bind(classOf[EventBus]) to classOf[SchedulerEventBus] in SINGLETON
    provideSingleton[SchedulerThreadCallQueue] { new SchedulerThreadCallQueue(new StandardCallQueue, cppProxy, schedulerThread) }
    bindInstance(controllerBridge.getEventBus: SchedulerEventBus )
    provideSingleton { new SchedulerInstanceId(randomUUID.toString) }
    provideSingleton { new DisposableCppProxyRegister }
    bindInstance(cppProxy.log.getSister: PrefixLog )
    provideSingleton { new SchedulerId(cppProxy.id) }
    provideSingleton { new ClusterMemberId(cppProxy.cluster_member_id) }
    provideSingleton { new DatabaseSubsystem(cppProxy.db) }
    provideSingleton { cppProxy.variables.getSister: VariableSet }
    bindSubsystems()
  }

  private def bindSubsystems() {
    provideSingleton[Folder_subsystemC] { cppProxy.folder_subsystem }
    provideSingleton[Job_subsystemC] { cppProxy.job_subsystem }
    provideSingleton[Lock_subsystemC] { cppProxy.lock_subsystem }
    provideSingleton[Order_subsystemC] { cppProxy.order_subsystem }
    provideSingleton[Process_class_subsystemC] { cppProxy.process_class_subsystem }
    provideSingleton[Schedule_subsystemC] { cppProxy.schedule_subsystem }
    provideSingleton[Task_subsystemC] { cppProxy.task_subsystem }
    provideSingleton[Standing_order_subsystemC] { cppProxy.standing_order_subsystem }
    provideSingleton[FileBasedSubsystem.Register] { new FileBasedSubsystem.Register(List(
      FolderSubsystem,
      JobSubsystem,
      LockSubsystem,
      OrderSubsystem,
      ProcessClassSubsystem,
      ScheduleSubsystem,
      StandingOrderSubsystem)) }
  }

  @Provides @Singleton def provideEntityManagerFactory(databaseSubsystem: DatabaseSubsystem): EntityManagerFactory =
    databaseSubsystem.entityManagerFactory

  @Provides @Singleton def provideSchedulerClusterMemberKey(schedulerId: SchedulerId, clusterMemberId: ClusterMemberId) =
    new SchedulerClusterMemberKey(schedulerId, clusterMemberId)

  @Provides @Singleton def provideCommandSubsystem(pluginSubsystem: PluginSubsystem) =
    new CommandSubsystem(asJavaIterable(commandHandlers(List(pluginSubsystem))))
}

object SchedulerModule {
  private def commandHandlers(objects: Iterable[AnyRef]): Iterable[CommandHandler] =
    (objects collect { case o: HasCommandHandlers => o.commandHandlers: Iterable[CommandHandler] }).flatten
}
