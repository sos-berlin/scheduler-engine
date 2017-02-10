package com.sos.scheduler.engine.kernel

import com.google.common.base.MoreObjects.firstNonNull
import com.google.inject
import com.google.inject.Guice.createInjector
import com.google.inject.Stage.DEVELOPMENT
import com.google.inject.{Injector, TypeLiteral}
import com.sos.scheduler.engine.client.command.SchedulerClientFactory
import com.sos.scheduler.engine.common.async.{CallQueue, CallRunner}
import com.sos.scheduler.engine.common.configutils.Configs.ConvertibleConfig
import com.sos.scheduler.engine.common.guice.GuiceImplicits._
import com.sos.scheduler.engine.common.log.LoggingFunctions.enableJavaUtilLoggingOverSLF4J
import com.sos.scheduler.engine.common.maven.MavenProperties
import com.sos.scheduler.engine.common.scalautil.Futures.awaitResult
import com.sos.scheduler.engine.common.scalautil.Futures.implicits.SuccessFuture
import com.sos.scheduler.engine.common.scalautil.xmls.SafeXML
import com.sos.scheduler.engine.common.scalautil.{HasCloser, Logger, SetOnce}
import com.sos.scheduler.engine.common.soslicense.LicenseKeyString
import com.sos.scheduler.engine.common.system.SystemInformations.systemInformation
import com.sos.scheduler.engine.common.time.ScalaTime.MaxDuration
import com.sos.scheduler.engine.common.utils.JavaResource
import com.sos.scheduler.engine.common.xml.DomForScala._
import com.sos.scheduler.engine.common.xml.XmlUtils.loadXml
import com.sos.scheduler.engine.cplusplus.runtime.annotation.ForCpp
import com.sos.scheduler.engine.cplusplus.runtime.{CppProxy, CppProxyInvalidatedException, DisposableCppProxyRegister, Sister}
import com.sos.scheduler.engine.data.event.KeyedEvent
import com.sos.scheduler.engine.data.filebased.{FileBasedEvent, FileBasedType}
import com.sos.scheduler.engine.data.scheduler.{SchedulerClosed, SchedulerInitiated, SchedulerOverview, SchedulerState}
import com.sos.scheduler.engine.data.system.JavaInformation
import com.sos.scheduler.engine.data.xmlcommands.XmlCommand
import com.sos.scheduler.engine.eventbus.{EventBus, SchedulerEventBus}
import com.sos.scheduler.engine.kernel.Scheduler._
import com.sos.scheduler.engine.kernel.async.SchedulerThreadFutures.{directOrSchedulerThreadFuture, inSchedulerThread}
import com.sos.scheduler.engine.kernel.async.{CppCall, SchedulerThreadCallQueue}
import com.sos.scheduler.engine.kernel.command.{CommandSubsystem, UnknownCommandException}
import com.sos.scheduler.engine.kernel.configuration.SchedulerModule
import com.sos.scheduler.engine.kernel.configuration.SchedulerModule.LateBoundCppSingletons
import com.sos.scheduler.engine.kernel.cppproxy.SpoolerC
import com.sos.scheduler.engine.kernel.database.DatabaseSubsystem
import com.sos.scheduler.engine.kernel.event.EventSubsystem
import com.sos.scheduler.engine.kernel.filebased.FileBasedSubsystem
import com.sos.scheduler.engine.kernel.job.TaskSubsystemClient
import com.sos.scheduler.engine.kernel.log.PrefixLog
import com.sos.scheduler.engine.kernel.plugin.{PluginModule, PluginSubsystem}
import com.sos.scheduler.engine.kernel.scheduler.SchedulerXmlCommandExecutor.Result
import com.sos.scheduler.engine.kernel.scheduler._
import com.sos.scheduler.engine.kernel.security.SchedulerSecurityLevel
import com.sos.scheduler.engine.kernel.settings.CppSettingName
import com.sos.scheduler.engine.kernel.time.TimeZones
import com.sos.scheduler.engine.main.SchedulerControllerBridge
import com.typesafe.config.Config
import java.io.ByteArrayInputStream
import java.lang.Thread.currentThread
import java.time.Instant.now
import java.time.ZoneId
import java.time.ZoneOffset.UTC
import javax.annotation.Nullable
import javax.inject.{Inject, Singleton}
import org.joda.time.DateTimeZone
import scala.collection.JavaConversions._
import scala.collection.immutable
import scala.concurrent.{ExecutionContext, Future}
import scala.util.control.NonFatal

@ForCpp
@Singleton
final class Scheduler @Inject private(
    cppProxy: SpoolerC,
    config: Config,
    controllerBridge: SchedulerControllerBridge,
    schedulerConfiguration: SchedulerConfiguration,
    prefixLog: PrefixLog,
    disposableCppProxyRegister: DisposableCppProxyRegister,
    implicit private val schedulerThreadCallQueue: SchedulerThreadCallQueue,
    eventBus: SchedulerEventBus,
    val injector: Injector)
extends Sister
with SchedulerIsClosed
with SchedulerXmlCommandExecutor
with HasInjector
with HasCloser {

  private var closed = false
  private val callRunner = new CallRunner(schedulerThreadCallQueue.delegate)
  private val mailDefaultsOnce = new SetOnce[Map[String, String]]
  lazy val mailDefaults: Map[String, String] = mailDefaultsOnce()
  private val startedAt = now()

  onClose { injector.instance[DependencyInjectionCloser].closer.close() }
  enableJavaUtilLoggingOverSLF4J()
  TimeZones.initialize()
  //DateTimeZone.setDefault(UTC);
  java.util.TimeZone.setDefault(java.util.TimeZone.getTimeZone(UTC))       // Für JPA @Temporal(TIMESTAMP), damit Date wirklich UTC enthält. Siehe http://stackoverflow.com/questions/508019
  Thread.currentThread.setContextClassLoader(getClass.getClassLoader)   // Für Mail, http://stackoverflow.com/questions/1969667

  cppProxy.setSister(this)

  private var pluginSubsystem: PluginSubsystem = null
  private var commandSubsystem: CommandSubsystem = null
  private var databaseSubsystem: DatabaseSubsystem = null
  private implicit var executionContext: ExecutionContext = null

  @ForCpp
  private def initialize(): Unit = {
    instantiateSubsystems()
    pluginSubsystem = injector.instance[PluginSubsystem]
    commandSubsystem = injector.instance[CommandSubsystem]
    databaseSubsystem = injector.instance[DatabaseSubsystem]
    executionContext = injector.instance[ExecutionContext]
    catchFileBasedEvents()
    prepareSettings()
  }

  private def prepareSettings(): Unit = {
    val settingsC = cppProxy.modifiable_settings
    if (settingsC._http_port.isEmpty) {
      for (httpPort ← config.optionAs[String]("jobscheduler.master.webserver.http.port"))
        settingsC.set(CppSettingName.HttpPort.number, httpPort)
    }
    if (settingsC._https_port.isEmpty) {
      for (httpPort ← config.optionAs[String]("jobscheduler.master.webserver.https.port"))
        settingsC.set(CppSettingName.HttpsPort.number, httpPort)
    }
  }

  private def instantiateSubsystems(): Unit = {
    for (companion ← injector.instance[FileBasedSubsystem.Register].companions) {
      injector.getInstance(companion.subsystemClass)
      injector.getInstance(companion.clientClass)
    }
    injector.instance[TaskSubsystemClient]
  }

  private def catchFileBasedEvents(): Unit = {
    val subsystemCompanions = injector.instance[FileBasedSubsystem.Register].companions
    val subsystemMap: Map[FileBasedType, FileBasedSubsystem] =
      subsystemCompanions.map { o ⇒ o.fileBasedType → injector.getInstance(o.subsystemClass) } .toMap
    eventBus.onHot[FileBasedEvent] {
      case KeyedEvent(path, event) ⇒
        for (subsystem ← subsystemMap.get(path.fileBasedType)) {
          subsystem.onFileBasedEvent(KeyedEvent(event)(path))
        }
    }
  }

  def onCppProxyInvalidated(): Unit = {}

  @ForCpp private def onClose(): Unit = {
    closed = true
    try {
      schedulerThreadCallQueue.close()
      eventBus.publish(KeyedEvent(SchedulerClosed))
      eventBus.dispatchEvents()
      schedulerThreadCallQueue.close()
      try databaseSubsystem.close() catch { case NonFatal(t) ⇒
        logger.error(s"databaseSubsystem.close(): $t", t)
      }
      try pluginSubsystem.close() catch { case NonFatal(t) ⇒
        logger.error(s"pluginSubsystem.close(): $t", t)
      }
    }
    finally {
      eventBus.dispatchEvents()
      disposableCppProxyRegister.tryDisposeAll()
      super.close()
    }
  }

  @ForCpp private def onSchedulerLoaded(): Unit = {
    injector.instance[SchedulerConfiguration].initialize()
    pluginSubsystem.initialize()
  }

  @ForCpp private def onLoad(): Unit = {
    // Actually, we are called at Scheduler::activate() - after onSchedulerLoaded
    mailDefaultsOnce := (cppProxy.mailDefaults map { _.split("=", 2) } map { case Array(k, v) ⇒ k → v }).toMap
    controllerBridge.onSchedulerStarted(this)
  }

  @ForCpp private def onDatabaseOpened(): Unit = databaseSubsystem.onDatabaseOpened()

  @ForCpp private def onActivate(): Unit = {
    initializeCppDependencySingletons()
    pluginSubsystem.activate()
  }

  private def initializeCppDependencySingletons(): Unit = {
    // Eagerly call all C++ providers now to avoid later deadlock (Scheduler lock and DI lock)
    for (o ← injector.instance[LateBoundCppSingletons].interfaces) injector.getInstance(o)
    val t = new TypeLiteral[immutable.Iterable[LicenseKeyString]] {}
    injector.getInstance(inject.Key.get(t))
  }

  @ForCpp private def onActivated(): Unit = {
    controllerBridge.onSchedulerActivated()
  }

  /** Wird bei jedem Schleifendurchlauf aufgerufen. */
  @ForCpp private def onEnteringSleepState(): Long = {
    val somethingDone = executeCallQueue()
    eventBus.dispatchEvents()
    val nextTime = schedulerThreadCallQueue.nextTime
    if (somethingDone) -nextTime else nextTime
  }

  def executeCallQueue() = inSchedulerThread { callRunner.executeMatureCalls() }

  /** Nur für C++, zur Ausführung eines Kommandos in Java */
  @ForCpp private def javaExecuteXml(xml: String): String = {
    try commandSubsystem.executeXml(xml)
    catch {
      case x: UnknownCommandException ⇒
        prefixLog.warn(x.toString)
        "UNKNOWN_COMMAND"   // Siehe command_error.cxx, für ordentliche Meldung SCHEDULER-105, bis Java die selbst liefert kann.
    }
  }

  @ForCpp private def sendCommandAndReplyToStout(uri: String, bytes: Array[Byte]): Unit = {
    val future = injector.instance[SchedulerClientFactory].apply(uri).uncheckedExecute(SafeXML.load(new ByteArrayInputStream(bytes)))
    val response: String = awaitResult(future, MaxDuration)
    System.out.println(response)
  }

  @ForCpp private def getEventSubsystem =
    injector.getInstance(classOf[EventSubsystem])

  @ForCpp private def uri: String =
    pluginSubsystem.stateOption(SosSpoolerUriName) getOrElse ""

  @ForCpp private def enqueueCall(o: CppCall): Unit = {
    schedulerThreadCallQueue.add(o)
  }

  @ForCpp private def cancelCall(o: CppCall): Unit = {
    schedulerThreadCallQueue.tryCancel(o)
  }

  def terminate(): Unit = {
    if (!isClosed) {
      try
        directOrSchedulerThreadFuture {
          if (!isClosed) {
            try cppProxy.cmd_terminate()
            catch {
              case x: CppProxyInvalidatedException ⇒
                logger.trace("Scheduler.terminate() ignored because C++ object has already been destroyed")
            }
          }
        }
      catch {
        case e: CallQueue.ClosedException ⇒ logger.debug(s"Ignored: $e")
      }
      // Return immediately, discarding the future
    }
  }

  def executeXmls(e: Iterable[scala.xml.NodeBuffer]): Result = executeXmlString(<commands>{e}</commands>.toString())

  def executeXml(o: XmlCommand): Result = executeXmlString(o.xmlString)

  def executeXmls(e: scala.xml.NodeSeq): Result = executeXmlString(<commands>{e}</commands>.toString())

  def executeXml(e: scala.xml.Elem): Result = executeXmlString(e.toString())

  private def executeXmlString(o: String) = Result(executeXml(o))

  /** Löst bei einem ERROR-Element eine Exception aus. */
  def executeXml(xml: String): String =
    executeXmlFuture(xml) await MaxDuration

  /** Löst bei einem ERROR-Element eine Exception aus. */
  def executeXmlFuture(xml: String): Future[String] =
    uncheckedExecuteXmlFuture(xml) map { result ⇒
      if (result contains "<ERROR") {
        for (error ← loadXml(result).getDocumentElement.childElements / "ERROR")
          throw new SchedulerException(error.getAttribute("text"))
      }
      result
    }

  /** execute_xml_string() der C++-Klasse Spooler */
  def uncheckedExecuteXml(xml: String): String =
    uncheckedExecuteXmlFuture(xml) await MaxDuration

  /** execute_xml_string() der C++-Klasse Spooler */
  def uncheckedExecuteXmlFuture(xml: String): Future[String] = {
    if (closed) sys.error("Scheduler is closed")
    directOrSchedulerThreadFuture { cppProxy.execute_xml_string(xml) } map {
      _ stripSuffix "\u0000"  // Von C++ angehängtes '\0', siehe Command_response::end_standard_response()
    }
  }

  def uncheckedExecuteXml(xml: String, securityLevel: SchedulerSecurityLevel, clientHostName: String) =
    uncheckedExecuteXmlFuture(xml, securityLevel, clientHostName) await MaxDuration

  def uncheckedExecuteXmlFuture(xml: String, securityLevel: SchedulerSecurityLevel, clientHostName: String): Future[String] =
    directOrSchedulerThreadFuture {
      cppProxy.execute_xml_string_with_security_level(xml, securityLevel.cppName, clientHostName)
    } map {
      _ stripSuffix "\u0000"  // Von C++ angehängtes '\0', siehe Command_response::end_standard_response()
    }

  def callCppAndDoNothing(): Unit = inSchedulerThread { cppProxy.tcp_port }

  private[kernel] def overview: SchedulerOverview =
    new SchedulerOverview(
      version = mavenProperties.buildVersion,
      startedAt = startedAt,
      schedulerId = schedulerConfiguration.schedulerId,
      httpPort = schedulerConfiguration.httpPortOption,
      httpsPort = schedulerConfiguration.httpsPortOption,
      udpPort = schedulerConfiguration.udpPort,
      supervisor = schedulerConfiguration.supervisorUriOption,
      pid = cppProxy.pid,
      state = SchedulerState.values()(cppProxy.state),
      system = systemInformation(),
      java = JavaInformation())

  def isClosed = closed
}

@ForCpp
object Scheduler {
  private val logger = Logger(getClass)
  private val mavenProperties = new MavenProperties(JavaResource("com/sos/scheduler/engine/kernel/maven.properties"))
  private val _defaultTimezoneId = DateTimeZone.getDefault.getID
  val DefaultZoneId = ZoneId.of(_defaultTimezoneId)

  /** Value for API sos.spooler.Spooler#uri */
  val SosSpoolerUriName = "sos.spooler.Spooler.uri"

  @ForCpp
  def defaultTimezoneId: String = _defaultTimezoneId

  @ForCpp def newInjector(cppProxy: SpoolerC, @Nullable controllerBridgeOrNull: SchedulerControllerBridge, configurationXml: String): Injector = {
    val controllerBridge = firstNonNull(controllerBridgeOrNull, EmptySchedulerControllerBridge.singleton)
    CppProxy.cppThreadStarted()  // Needed, when process has been started via C++ (which is the case in production)
    controllerBridge.cppSettings.setSettingsInCpp(cppProxy.modifiable_settings)
    val injector = createInjector(DEVELOPMENT, List(   // Must be DEVELOPMENT for some singletons needs the started C++ Scheduler.
      new SchedulerModule(cppProxy, controllerBridge, schedulerThread = currentThread),
      PluginModule(configurationXml)))
    controllerBridge.setInjector(injector)
    injector.instance[EventBus].publish(SchedulerInitiated)
    injector
  }

  @ForCpp
  def buildVersion: String =
    try mavenProperties.buildVersion
    catch { case e: NoSuchElementException ⇒ "" }

  @ForCpp
  def versionCommitHash: String =
    if (mavenProperties.version endsWith "-SNAPSHOT")
      try mavenProperties.versionCommitHash
      catch { case e: NoSuchElementException ⇒ "" }
    else
      ""
}
