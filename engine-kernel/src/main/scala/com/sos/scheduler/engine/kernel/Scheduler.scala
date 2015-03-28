package com.sos.scheduler.engine.kernel

import com.google.common.base.MoreObjects.firstNonNull
import com.google.inject.Guice.createInjector
import com.google.inject.Injector
import com.sos.scheduler.engine.client.command.HttpSchedulerCommandClient
import com.sos.scheduler.engine.common.async.{CallQueue, CallRunner}
import com.sos.scheduler.engine.common.guice.GuiceImplicits._
import com.sos.scheduler.engine.common.log.LoggingFunctions.enableJavaUtilLoggingOverSLF4J
import com.sos.scheduler.engine.common.scalautil.Futures.awaitResult
import com.sos.scheduler.engine.common.scalautil.xmls.SafeXML
import com.sos.scheduler.engine.common.scalautil.{HasCloser, Logger}
import com.sos.scheduler.engine.common.utils.JavaResource
import com.sos.scheduler.engine.common.xml.NamedChildElements
import com.sos.scheduler.engine.common.xml.XmlUtils.{childElements, loadXml}
import com.sos.scheduler.engine.cplusplus.runtime.annotation.ForCpp
import com.sos.scheduler.engine.cplusplus.runtime.{CppProxy, CppProxyInvalidatedException, DisposableCppProxyRegister, Sister}
import com.sos.scheduler.engine.data.filebased.{FileBasedEvent, FileBasedType}
import com.sos.scheduler.engine.data.log.SchedulerLogLevel
import com.sos.scheduler.engine.data.scheduler.SchedulerCloseEvent
import com.sos.scheduler.engine.data.xmlcommands.XmlCommand
import com.sos.scheduler.engine.eventbus.{EventSubscription, SchedulerEventBus}
import com.sos.scheduler.engine.kernel.Scheduler._
import com.sos.scheduler.engine.kernel.async.SchedulerThreadFutures.{directOrSchedulerThreadFuture, inSchedulerThread}
import com.sos.scheduler.engine.kernel.async.{CppCall, SchedulerThreadCallQueue}
import com.sos.scheduler.engine.kernel.command.{CommandSubsystem, UnknownCommandException}
import com.sos.scheduler.engine.kernel.configuration.SchedulerModule
import com.sos.scheduler.engine.kernel.configuration.SchedulerModule.LazyBoundCppSingletons
import com.sos.scheduler.engine.kernel.cppproxy.SpoolerC
import com.sos.scheduler.engine.kernel.database.DatabaseSubsystem
import com.sos.scheduler.engine.kernel.event.EventSubsystem
import com.sos.scheduler.engine.kernel.filebased.FileBasedSubsystem
import com.sos.scheduler.engine.kernel.log.{CppLogger, PrefixLog}
import com.sos.scheduler.engine.kernel.plugin.{PluginModule, PluginSubsystem}
import com.sos.scheduler.engine.kernel.scheduler.SchedulerXmlCommandExecutor.Result
import com.sos.scheduler.engine.kernel.scheduler._
import com.sos.scheduler.engine.kernel.security.SchedulerSecurityLevel
import com.sos.scheduler.engine.kernel.time.TimeZones
import com.sos.scheduler.engine.kernel.util.MavenProperties
import com.sos.scheduler.engine.main.SchedulerControllerBridge
import java.io.ByteArrayInputStream
import java.lang.Thread.currentThread
import javax.annotation.Nullable
import javax.inject.{Inject, Singleton}
import org.joda.time.DateTimeZone
import org.joda.time.DateTimeZone.UTC
import org.joda.time.Instant.now
import scala.collection.JavaConversions._
import scala.collection.breakOut
import scala.concurrent.duration.Duration
import scala.util.control.NonFatal

@ForCpp
@Singleton
final class Scheduler @Inject private(
    cppProxy: SpoolerC,
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
  private lazy val pluginSubsystem = injector.apply[PluginSubsystem]
  private lazy val commandSubsystem = injector.apply[CommandSubsystem]
  private lazy val databaseSubsystem = injector.apply[DatabaseSubsystem]

  val startInstant = now()

  onClose { injector.apply[DependencyInjectionCloser].closer.close() }
  enableJavaUtilLoggingOverSLF4J()
  TimeZones.initialize()
  //DateTimeZone.setDefault(UTC);
  java.util.TimeZone.setDefault(UTC.toTimeZone)       // Für JPA @Temporal(TIMESTAMP), damit Date wirklich UTC enthält. Siehe http://stackoverflow.com/questions/508019
  Thread.currentThread.setContextClassLoader(getClass.getClassLoader)   // Für Mail, http://stackoverflow.com/questions/1969667

  cppProxy.setSister(this)

  if (controllerBridge eq EmptySchedulerControllerBridge.singleton) { // Wenn wir ein controllerBridge haben, ist der Scheduler über Java (CppScheduler.main) aufgerufen worden. Dort wird die Sperre gesetzt.
    threadLock()
    onClose { threadUnlock() } //TODO Sperre wird in onClose() zu früh freigegeben, der Scheduler läuft ja noch. Lösung: Start über Java mit CppScheduler.run()
  }

  @ForCpp
  private def initialize(): Unit = {
    val eventSubscription = {
      val subsystemDescriptions = injector.apply[FileBasedSubsystem.Register].descriptions
      val subsystemMap: Map[FileBasedType, FileBasedSubsystem] = subsystemDescriptions.map { o ⇒ o.fileBasedType -> injector.getInstance(o.subsystemClass) }(breakOut)
      EventSubscription[FileBasedEvent] { e ⇒ for (subsystem <- subsystemMap.get(e.typedPath.fileBasedType)) subsystem.onFileBasedEvent(e) }
    }
    eventBus.registerHot(eventSubscription)
    onClose { eventBus.unregisterHot(eventSubscription) }
  }

  def onCppProxyInvalidated(): Unit = {}

  @ForCpp private def onClose(): Unit = {
    closed = true
    try {
      schedulerThreadCallQueue.close()
      eventBus.publish(new SchedulerCloseEvent)
      eventBus.dispatchEvents()
      schedulerThreadCallQueue.close()
      try databaseSubsystem.close() catch { case NonFatal(x) ⇒ prefixLog.error(s"databaseSubsystem.close(): $x") }
      try pluginSubsystem.close() catch { case NonFatal(x) ⇒ prefixLog.error(s"pluginSubsystem.close(): $x") }
    }
    finally {
      eventBus.dispatchEvents()
      disposableCppProxyRegister.tryDisposeAll()
      super.close()
    }
  }

  @ForCpp private def onLoad(): Unit = {
    pluginSubsystem.initialize()
    controllerBridge.onSchedulerStarted(this)
  }

  @ForCpp private def onActivate(): Unit = {
    initializeCppDependencySingletons()
    pluginSubsystem.activate()
  }

  private def initializeCppDependencySingletons(): Unit = {
    // Eagerly call all C++ providers now to avoid later deadlock (Scheduler lock and DI lock)
    for (o ← injector.apply[LazyBoundCppSingletons].interfaces) injector.getInstance(o)
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

  def executeCallQueue() = callRunner.executeMatureCalls()

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
    val future = injector.apply[HttpSchedulerCommandClient].uncheckedExecute(uri, SafeXML.load(new ByteArrayInputStream(bytes)))
    val response: String = awaitResult(future, Duration.Inf)
    System.out.println(response)
  }

  @ForCpp private def getEventSubsystem =
    injector.getInstance(classOf[EventSubsystem])

  @ForCpp private def log(prefix: String, level: Int, line: String): Unit = {
    CppLogger.log(prefix, SchedulerLogLevel.ofCpp(level), line)
  }

  @ForCpp private def enqueueCall(o: CppCall): Unit = {
    schedulerThreadCallQueue.add(o)
  }

  @ForCpp private def cancelCall(o: CppCall): Unit = {
    schedulerThreadCallQueue.tryCancel(o)
  }

  @ForCpp private def threadLock(): Unit = {
    CppProxy.threadLock.lock()
  }

  @ForCpp /*private*/ def threadUnlock(): Unit = {
    CppProxy.threadLock.unlock()
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
  def executeXml(xml: String): String = {
    val result = uncheckedExecuteXml(xml)
    if (result contains "<ERROR") {
      for (e <- childElements(loadXml(result).getDocumentElement);
           error <- new NamedChildElements("ERROR", e))
        throw new SchedulerException(error.getAttribute("text"))
    }
    result
  }

  /** execute_xml_string() der C++-Klasse Spooler */
  def uncheckedExecuteXml(xml: String): String = {
    if (closed) sys.error("Scheduler is closed")
    inSchedulerThread { cppProxy.execute_xml_string(xml) }
    .stripSuffix("\u0000")  // Von C++ angehängtes '\0' an, siehe Command_response::end_standard_response()
  }

  def uncheckedExecuteXml(xml: String, securityLevel: SchedulerSecurityLevel, clientHostName: String) =
    inSchedulerThread { cppProxy.execute_xml_string_with_security_level(xml, securityLevel.cppName, clientHostName) }
    .stripSuffix("\u0000")  // Von C++ angehängtes '\0' an, siehe Command_response::end_standard_response()

  //    /** @param text Sollte auf \n enden */
  //    public void writeToSchedulerLog(LogCategory category, String text) {
  //        cppProxy.write_to_scheduler_log(category.string(), text);
  //    }

  def callCppAndDoNothing(): Unit = {
    cppProxy.tcp_port
  }

  def overview: SchedulerOverview =
    inSchedulerThread {
      new SchedulerOverview(
        version = mavenProperties.version,
        versionCommitHash = mavenProperties.versionCommitHash,
        startInstant = startInstant,
        instant = now(),
        schedulerId = schedulerConfiguration.schedulerId,
        tcpPort = schedulerConfiguration.tcpPort match { case 0 ⇒ None case n ⇒ Some(n) },
        udpPort = schedulerConfiguration.udpPort,
        processId = cppProxy.pid,
        state = cppProxy.state_name)
    }

  def isClosed = closed
}

@ForCpp
object Scheduler {
  private val logger = Logger(getClass)
  private val mavenProperties = new MavenProperties(JavaResource("com/sos/scheduler/engine/kernel/maven.properties"))
  private val _defaultTimezoneId = DateTimeZone.getDefault.getID

  @ForCpp
  def defaultTimezoneId: String = _defaultTimezoneId

  @ForCpp def newInjector(cppProxy: SpoolerC, @Nullable controllerBridgeOrNull: SchedulerControllerBridge, configurationXml: String) = {
    val controllerBridge = firstNonNull(controllerBridgeOrNull, EmptySchedulerControllerBridge.singleton)
    controllerBridge.cppSettings.setSettingsInCpp(cppProxy.modifiable_settings)
    createInjector(Seq(
      new SchedulerModule(cppProxy, controllerBridge, schedulerThread = currentThread),
      PluginModule(configurationXml)))
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
