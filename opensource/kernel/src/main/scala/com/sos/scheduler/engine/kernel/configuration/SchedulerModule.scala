package com.sos.scheduler.engine.kernel.configuration

import com.google.inject._
import com.sos.scheduler.engine.cplusplus.runtime.DisposableCppProxyRegister
import com.sos.scheduler.engine.data.scheduler.ClusterMemberId
import com.sos.scheduler.engine.data.scheduler.SchedulerClusterMemberKey
import com.sos.scheduler.engine.data.scheduler.SchedulerId
import com.sos.scheduler.engine.eventbus.{EventBus, SchedulerEventBus}
import com.sos.scheduler.engine.kernel.Scheduler
import com.sos.scheduler.engine.kernel.command.{CommandHandler, HasCommandHandlers, CommandSubsystem}
import com.sos.scheduler.engine.kernel.cppproxy.SpoolerC
import com.sos.scheduler.engine.kernel.database.DatabaseSubsystem
import com.sos.scheduler.engine.kernel.event.{OperationQueue, OperationExecutor, EventSubsystem}
import com.sos.scheduler.engine.kernel.folder.FolderSubsystem
import com.sos.scheduler.engine.kernel.job.JobSubsystem
import com.sos.scheduler.engine.kernel.log.PrefixLog
import com.sos.scheduler.engine.kernel.order.OrderSubsystem
import com.sos.scheduler.engine.kernel.plugin.PluginSubsystem
import com.sos.scheduler.engine.kernel.scheduler._
import com.sos.scheduler.engine.main.SchedulerControllerBridge
import java.util.UUID.randomUUID
import scala.collection.JavaConversions._

class SchedulerModule(cppProxy: SpoolerC, controllerBridge: SchedulerControllerBridge, scheduler: Scheduler) extends AbstractModule {

  import SchedulerModule._

  private val instanceId = new SchedulerInstanceId(randomUUID.toString)
  private val eventBus = controllerBridge.getEventBus
  private val configuration = new SchedulerConfiguration(cppProxy)
  private val _log = cppProxy.log.getSister
  private val operationExecutor = new OperationExecutor(_log)
  private val databaseSubsystem = new DatabaseSubsystem(this.cppProxy.db)
  private val eventSubsystem = new EventSubsystem(eventBus)
  private val folderSubsystem = new FolderSubsystem(this.cppProxy.folder_subsystem)
  private val jobSubsystem = new JobSubsystem(this.cppProxy.job_subsystem)
  private val orderSubsystem = new OrderSubsystem(this.cppProxy.order_subsystem)
  private val disposableCppProxyRegister = new DisposableCppProxyRegister

  def configure() {
    bind(classOf[SpoolerC]).toInstance(cppProxy)
    bind(classOf[EventBus]).toInstance(eventBus)
    bind(classOf[SchedulerEventBus]).toInstance(eventBus)
    bind(classOf[SchedulerInstanceId]).toInstance(instanceId)
    bind(classOf[DatabaseSubsystem]).toInstance(databaseSubsystem)
    bind(classOf[DisposableCppProxyRegister]).toInstance(disposableCppProxyRegister)
    bind(classOf[FolderSubsystem]).toInstance(folderSubsystem)
    bind(classOf[JobSubsystem]).toInstance(jobSubsystem)
    bind(classOf[OrderSubsystem]).toInstance(orderSubsystem)
    bind(classOf[EventSubsystem]).toInstance(eventSubsystem)
    bind(classOf[OperationQueue]).to(classOf[OperationExecutor])
    bind(classOf[OperationExecutor]).toInstance(operationExecutor)
    bind(classOf[PrefixLog]).toInstance(_log)
    bind(classOf[Scheduler]).toInstance(scheduler)
    bind(classOf[SchedulerConfiguration]).toInstance(configuration)
    bind(classOf[SchedulerHttpService]).toInstance(scheduler)
    bind(classOf[SchedulerInstanceId]).toInstance(instanceId)
    bind(classOf[SchedulerIsClosed]).toInstance(scheduler)
    bind(classOf[SchedulerXmlCommandExecutor]).toInstance(scheduler)
    bind(classOf[SchedulerModule]).toInstance(this)  // Nur für JettyPlugin, bis es unseren Injector nutzen kann (setzt Jetty-Initialisierung vor Guice voraus)
  }

//  private def provide[A <: AnyRef](provider: => A)(implicit c: ClassTag) = {
//    bind(c.runtimeClass.asInstanceOf[Class[A]]).toProvider(new Provider[A] {
//      def get() = provider
//    })
//  }

//  @Provides @Singleton def provideOperationQueue(prefixLog: PrefixLog) =
//    new OperationExecutor(prefixLog)

  @Provides @Singleton def provideClusterMemberId =
    new ClusterMemberId(cppProxy.cluster_member_id)

  @Provides @Singleton def provideEntityManagerFactory(databaseSubsystem: DatabaseSubsystem) =
    databaseSubsystem.entityManagerFactory

  @Provides @Singleton def provideEntityManager(databaseSubsystem: DatabaseSubsystem) =
    databaseSubsystem.entityManagerFactory.createEntityManager

  @Provides @Singleton def provideSchedulerId =
    new SchedulerId(cppProxy.id)

  @Provides @Singleton def provideSchedulerClusterMemberKey(schedulerId: SchedulerId, clusterMemberId: ClusterMemberId) =
    new SchedulerClusterMemberKey(schedulerId, clusterMemberId)

  @Provides @Singleton def provideCommandSubsystem(pluginSubsystem: PluginSubsystem) =
    new CommandSubsystem(asJavaIterable(commandHandlers(List(pluginSubsystem))))
}

object SchedulerModule {
  private def commandHandlers(objects: Iterable[AnyRef]) =
    (objects collect { case o: HasCommandHandlers => o.getCommandHandlers: Iterable[CommandHandler] }).flatten
}