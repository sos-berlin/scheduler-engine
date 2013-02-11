package com.sos.scheduler.engine.kernel.configuration

import com.google.inject.Provides
import com.google.inject.Scopes.SINGLETON
import com.sos.scheduler.engine.cplusplus.runtime.DisposableCppProxyRegister
import com.sos.scheduler.engine.data.scheduler.ClusterMemberId
import com.sos.scheduler.engine.data.scheduler.SchedulerClusterMemberKey
import com.sos.scheduler.engine.data.scheduler.SchedulerId
import com.sos.scheduler.engine.eventbus.{EventBus, SchedulerEventBus}
import com.sos.scheduler.engine.kernel.command.{CommandHandler, HasCommandHandlers, CommandSubsystem}
import com.sos.scheduler.engine.kernel.cppproxy.SpoolerC
import com.sos.scheduler.engine.kernel.database.DatabaseSubsystem
import com.sos.scheduler.engine.kernel.event.{OperationQueue, OperationExecutor}
import com.sos.scheduler.engine.kernel.folder.FolderSubsystem
import com.sos.scheduler.engine.kernel.job.JobSubsystem
import com.sos.scheduler.engine.kernel.order.OrderSubsystem
import com.sos.scheduler.engine.kernel.plugin.PluginSubsystem
import com.sos.scheduler.engine.kernel.scheduler._
import com.sos.scheduler.engine.main.SchedulerControllerBridge
import java.util.UUID.randomUUID
import javax.inject.Singleton
import scala.collection.JavaConversions._

class SchedulerModule(cppProxy: SpoolerC, controllerBridge: SchedulerControllerBridge) extends ScalaAbstractModule {

  import SchedulerModule._

  def configure() {
    bindInstance(cppProxy)
    bindInstance(controllerBridge)
    bind(classOf[EventBus]) to classOf[SchedulerEventBus] in SINGLETON
    bindInstance(controllerBridge.getEventBus)
    provideSingleton { new SchedulerInstanceId(randomUUID.toString) }
    provideSingleton { new DisposableCppProxyRegister }
    bind(classOf[OperationQueue]) to classOf[OperationExecutor] in SINGLETON
    bindInstance(cppProxy.log.getSister)
    bindInstance(this) // Nur für JettyPlugin, bis es unseren Injector nutzen kann (setzt Jetty-Initialisierung vor Guice voraus)
    provideSingleton { new SchedulerId(cppProxy.id) }
    provideSingleton { new ClusterMemberId(cppProxy.cluster_member_id) }
    provideSingleton { new FolderSubsystem(cppProxy.folder_subsystem) }
    provideSingleton { new JobSubsystem(cppProxy.job_subsystem) }
    provideSingleton { new OrderSubsystem(cppProxy.order_subsystem) }
    provideSingleton { new DatabaseSubsystem(cppProxy.db) }
    provideSingleton { cppProxy.variables.getSister }
  }

  @Provides @Singleton def provideEntityManagerFactory(databaseSubsystem: DatabaseSubsystem) =
    databaseSubsystem.entityManagerFactory

  @Provides @Singleton def provideEntityManager(databaseSubsystem: DatabaseSubsystem) =
    databaseSubsystem.entityManagerFactory.createEntityManager

  @Provides @Singleton def provideSchedulerClusterMemberKey(schedulerId: SchedulerId, clusterMemberId: ClusterMemberId) =
    new SchedulerClusterMemberKey(schedulerId, clusterMemberId)

  @Provides @Singleton def provideCommandSubsystem(pluginSubsystem: PluginSubsystem) =
    new CommandSubsystem(asJavaIterable(commandHandlers(List(pluginSubsystem))))
}

object SchedulerModule {
  private def commandHandlers(objects: Iterable[AnyRef]): Iterable[CommandHandler] =
    (objects collect { case o: HasCommandHandlers => o.getCommandHandlers: Iterable[CommandHandler] }).flatten
}