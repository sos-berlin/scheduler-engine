package com.sos.scheduler.engine.kernel.configuration

import akka.actor.{ActorRefFactory, ActorSystem}
import com.google.common.base.Splitter
import com.google.inject.Scopes.SINGLETON
import com.google.inject.{Injector, Provides}
import com.sos.scheduler.engine.base.utils.ScalaUtils.implicitClass
import com.sos.scheduler.engine.common.akkautils.DeadLetterActor
import com.sos.scheduler.engine.common.async.StandardCallQueue
import com.sos.scheduler.engine.common.auth.EncodedPasswordValidator
import com.sos.scheduler.engine.common.configutils.Configs.parseConfigIfExists
import com.sos.scheduler.engine.common.event.collector.EventCollector
import com.sos.scheduler.engine.common.guice.ScalaAbstractModule
import com.sos.scheduler.engine.common.scalautil.Closers.implicits._
import com.sos.scheduler.engine.common.scalautil.FileUtils.implicits._
import com.sos.scheduler.engine.common.scalautil.HasCloser
import com.sos.scheduler.engine.common.soslicense.LicenseKeyString
import com.sos.scheduler.engine.common.sprayutils.web.auth.{CSRF, GateKeeper}
import com.sos.scheduler.engine.common.time.timer.TimerService
import com.sos.scheduler.engine.cplusplus.runtime.DisposableCppProxyRegister
import com.sos.scheduler.engine.data.scheduler.{ClusterMemberId, SchedulerClusterMemberKey, SchedulerId, SchedulerState, SupervisorUri}
import com.sos.scheduler.engine.eventbus.{EventBus, SchedulerEventBus}
import com.sos.scheduler.engine.kernel.DirectSchedulerClient
import com.sos.scheduler.engine.kernel.async.SchedulerThreadCallQueue
import com.sos.scheduler.engine.kernel.async.SchedulerThreadFutures.inSchedulerThread
import com.sos.scheduler.engine.kernel.command.{CommandHandler, CommandSubsystem, HasCommandHandlers, PublishEvent, Result}
import com.sos.scheduler.engine.kernel.configuration.SchedulerModule._
import com.sos.scheduler.engine.kernel.cppproxy._
import com.sos.scheduler.engine.kernel.database.{DatabaseSubsystem, JdbcConnectionPool}
import com.sos.scheduler.engine.kernel.event.collector.SchedulerEventCollector
import com.sos.scheduler.engine.kernel.filebased.FileBasedSubsystem
import com.sos.scheduler.engine.kernel.folder.FolderSubsystem
import com.sos.scheduler.engine.kernel.job.JobSubsystem
import com.sos.scheduler.engine.kernel.lock.LockSubsystem
import com.sos.scheduler.engine.kernel.log.PrefixLog
import com.sos.scheduler.engine.kernel.messagecode.MessageCodeHandler
import com.sos.scheduler.engine.kernel.order.{DirectOrderClient, OrderSubsystem, StandingOrderSubsystem}
import com.sos.scheduler.engine.kernel.plugin.PluginSubsystem
import com.sos.scheduler.engine.kernel.processclass.ProcessClassSubsystem
import com.sos.scheduler.engine.kernel.schedule.ScheduleSubsystem
import com.sos.scheduler.engine.kernel.scheduler._
import com.sos.scheduler.engine.kernel.security.AccessTokenRegister
import com.sos.scheduler.engine.kernel.variable.VariableSet
import com.sos.scheduler.engine.main.SchedulerControllerBridge
import com.typesafe.config.Config
import java.time.ZoneId
import java.util.UUID.randomUUID
import javax.inject.Singleton
import javax.persistence.EntityManagerFactory
import scala.collection.JavaConversions._
import scala.collection.{immutable, mutable}
import scala.concurrent.ExecutionContext
import scala.concurrent.duration._
import scala.reflect.ClassTag

final class SchedulerModule(spoolerC: SpoolerC, controllerBridge: SchedulerControllerBridge, schedulerThread: Thread)
extends ScalaAbstractModule
with HasCloser {

  private val lateBoundCppSingletons = mutable.Buffer[Class[_]]()

  override def configure(): Unit = {
    bind(classOf[DependencyInjectionCloser]) toInstance DependencyInjectionCloser(closer)
    bindInstance(spoolerC)
    bindInstance(controllerBridge)
    bind(classOf[EventBus]) to classOf[SchedulerEventBus] in SINGLETON
    bind(classOf[EventCollector]) to classOf[SchedulerEventCollector] in SINGLETON
    provideSingleton[SchedulerThreadCallQueue] { new SchedulerThreadCallQueue(new StandardCallQueue, spoolerC, schedulerThread) }
    bindInstance(controllerBridge.getEventBus: SchedulerEventBus)
    provideSingleton { SchedulerInstanceId(randomUUID.toString) }
    provideSingleton { new DisposableCppProxyRegister }
    bindInstance(spoolerC.log.getSister: PrefixLog )
    provideCppSingleton { new SchedulerId(spoolerC.id) }
    provideCppSingleton { new ClusterMemberId(spoolerC.cluster_member_id) }
    provideCppSingleton { spoolerC.variables.getSister: VariableSet }
    lateBoundCppSingletons += classOf[MessageCodeHandler]
    lateBoundCppSingletons += classOf[ZoneId]
    bindSubsystems()
    bindInstance(LateBoundCppSingletons(lateBoundCppSingletons.toVector))
  }

  private def bindSubsystems(): Unit = {
    provideCppSingleton[Folder_subsystemC] { spoolerC.folder_subsystem }
    provideCppSingleton[Job_subsystemC] { spoolerC.job_subsystem }
    provideCppSingleton[Lock_subsystemC] { spoolerC.lock_subsystem }
    provideCppSingleton[Order_subsystemC] { spoolerC.order_subsystem }
    provideCppSingleton[Process_class_subsystemC] { spoolerC.process_class_subsystem }
    provideCppSingleton[Schedule_subsystemC] { spoolerC.schedule_subsystem }
    provideCppSingleton[Task_subsystemC] { spoolerC.task_subsystem }
    provideCppSingleton[Standing_order_subsystemC] { spoolerC.standing_order_subsystem }
  }

  private def provideCppSingleton[A <: AnyRef : ClassTag](provider: ⇒ A) = {
    lateBoundCppSingletons += implicitClass[A]
    provideSingleton(provider)
  }

  @Provides @Singleton
  private def provideCsrfConfiguration(config: Config): CSRF.Configuration =
    CSRF.Configuration.fromSubConfig(config.getConfig("jobscheduler.master.webserver.csrf"))

  @Provides @Singleton
  private def provideEventCollectorConfiguration(config: Config): EventCollector.Configuration =
    EventCollector.Configuration.fromSubConfig(config.getConfig("jobscheduler.master.event"))

  @Provides @Singleton
  private def provideGateKeeperConfiguration(config: Config, accessTokenRegister: AccessTokenRegister): GateKeeper.Configuration =
    GateKeeper.Configuration.fromSubConfig(config.getConfig("jobscheduler.master.webserver.auth"))
      .copy(
        providePasswordValidator = () ⇒ EncodedPasswordValidator.fromSubConfig(config.getConfig("jobscheduler.master.auth.users")),
        provideAccessTokenValidator = () ⇒ accessTokenRegister.validate)

  @Provides @Singleton
  private def provideJdbcConnectionPool(config: Config, databaseSubsystem: DatabaseSubsystem, executionContext: ExecutionContext): JdbcConnectionPool =
    new JdbcConnectionPool(config, () ⇒ databaseSubsystem.cppProperties)(executionContext).closeWithCloser

  @Provides @Singleton
  private def provideDirectSchedulerCollector(o: DirectSchedulerClient): DirectOrderClient = o

  @Provides @Singleton
  private def provideSchedulerConfiguration(spoolerC: SpoolerC): SchedulerConfiguration =
    new SchedulerConfiguration.Injectable(spoolerC)

  @Provides @Singleton
  private def provideFileBasedSubsystemRegister(injector: Injector): FileBasedSubsystem.Register =
    FileBasedSubsystem.Register(injector, List(
      FolderSubsystem,
      JobSubsystem,
      LockSubsystem,
      OrderSubsystem,
      ProcessClassSubsystem,
      ScheduleSubsystem,
      StandingOrderSubsystem))

  @Provides @Singleton
  private def provideEntityManagerFactory(databaseSubsystem: DatabaseSubsystem)(implicit stcq: SchedulerThreadCallQueue): EntityManagerFactory =
    inSchedulerThread {
      databaseSubsystem.newEntityManagerFactory()
    }

  @Provides @Singleton
  private def provideDatabaseSubsystem(implicit stcq: SchedulerThreadCallQueue) =
    new DatabaseSubsystem(() ⇒ inSchedulerThread {
      spoolerC.db
    })

  @Provides @Singleton
  private def provideSchedulerClusterMemberKey(schedulerId: SchedulerId, clusterMemberId: ClusterMemberId) =
    new SchedulerClusterMemberKey(schedulerId, clusterMemberId)

  @Provides @Singleton
  private def provideCommandSubsystem(eventBus: EventBus, pluginSubsystem: PluginSubsystem) =
    new CommandSubsystem(asJavaIterable(
      PublishEvent.commandHandlers(eventBus) :::
      commandHandlers(List(pluginSubsystem))))

  @Provides @Singleton
  private def provideMessageCodeHandler(spoolerC: SpoolerC)(implicit stcq: SchedulerThreadCallQueue): MessageCodeHandler =
    inSchedulerThread {
      MessageCodeHandler.fromCodeAndTextStrings(spoolerC.settings.messageTexts)
    }

  @Provides @Singleton
  private def licenseKeyStrings(spoolerC: SpoolerC)(implicit stcq: SchedulerThreadCallQueue): immutable.Iterable[LicenseKeyString] =
    inSchedulerThread {
      Splitter.on(' ').omitEmptyStrings.splitToList(spoolerC.settings.installed_licence_keys_string).toVector.distinct map LicenseKeyString.apply
    }

  @Provides @Singleton
  private def zoneId(implicit stcq: SchedulerThreadCallQueue): ZoneId =
    inSchedulerThread {
      val state = SchedulerState.values()(spoolerC.state)
      if (Set(SchedulerState.none, SchedulerState.loading)(state)) throw new IllegalStateException(s"ZoneId while state=$state")
      ZoneId of spoolerC.time_zone_name
    }

  @Provides @Singleton
  private def actorSystem(config: Config): ActorSystem = {
    val actorSystem = ActorSystem("Engine", config)
    closer.onClose {
      actorSystem.shutdown()
      actorSystem.awaitTermination(30.seconds)
    }
    DeadLetterActor.subscribe(actorSystem)
    actorSystem
  }

  @Provides @Singleton
  private def provideSupervisorUriOption(conf: SchedulerConfiguration): Option[SupervisorUri] =
    conf.supervisorUriOption

  @Provides @Singleton
  private def config(conf: SchedulerConfiguration)(implicit stcq: SchedulerThreadCallQueue): Config =
    inSchedulerThread {
      parseConfigIfExists(conf.mainConfigurationDirectory / "private/private.conf")
        .withFallback(parseConfigIfExists(conf.mainConfigurationDirectory / "master.conf"))
        .withFallback(SchedulerConfiguration.DefaultConfig)
    }

  @Provides @Singleton
  private def executionContext(actorSystem: ActorSystem): ExecutionContext = actorSystem.dispatcher

  @Provides @Singleton
  private def actorRefFactory(actorSystem: ActorSystem): ActorRefFactory = actorSystem

  @Provides @Singleton
  private def timerService(implicit executionContext: ExecutionContext): TimerService = TimerService().closeWithCloser
}

object SchedulerModule {
  private def commandHandlers(objects: List[AnyRef]): List[CommandHandler] =
    (objects collect { case o: HasCommandHandlers ⇒ o.commandHandlers: Iterable[CommandHandler] }).flatten

  final case class LateBoundCppSingletons(interfaces: Vector[Class[_]])
}
