package com.sos.scheduler.engine.kernel

import Scheduler._
import com.google.common.base.Objects.firstNonNull
import com.google.inject.Guice.createInjector
import com.google.inject.Injector
import com.sos.scheduler.engine.common.async.CallRunner
import com.sos.scheduler.engine.common.log.LoggingFunctions.enableJavaUtilLoggingOverSLF4J
import com.sos.scheduler.engine.common.scalautil.Logger
import com.sos.scheduler.engine.common.xml.NamedChildElements
import com.sos.scheduler.engine.common.xml.XmlUtils.childElements
import com.sos.scheduler.engine.common.xml.XmlUtils.loadXml
import com.sos.scheduler.engine.cplusplus.runtime.CppProxy
import com.sos.scheduler.engine.cplusplus.runtime.CppProxyInvalidatedException
import com.sos.scheduler.engine.cplusplus.runtime.DisposableCppProxyRegister
import com.sos.scheduler.engine.cplusplus.runtime.Sister
import com.sos.scheduler.engine.cplusplus.runtime.annotation.ForCpp
import com.sos.scheduler.engine.data.log.SchedulerLogLevel
import com.sos.scheduler.engine.data.scheduler.SchedulerCloseEvent
import com.sos.scheduler.engine.eventbus.SchedulerEventBus
import com.sos.scheduler.engine.kernel.async.SchedulerThreadFutures.inSchedulerThread
import com.sos.scheduler.engine.kernel.async.{SchedulerThreadCallQueue, CppCall}
import com.sos.scheduler.engine.kernel.command.CommandSubsystem
import com.sos.scheduler.engine.kernel.command.UnknownCommandException
import com.sos.scheduler.engine.kernel.configuration.SchedulerModule
import com.sos.scheduler.engine.kernel.cppproxy.{HttpResponseC, SpoolerC}
import com.sos.scheduler.engine.kernel.database.DatabaseSubsystem
import com.sos.scheduler.engine.kernel.event.EventSubsystem
import com.sos.scheduler.engine.kernel.log.CppLogger
import com.sos.scheduler.engine.kernel.log.PrefixLog
import com.sos.scheduler.engine.kernel.plugin.{PluginModule, PluginSubsystem}
import com.sos.scheduler.engine.kernel.scheduler._
import com.sos.scheduler.engine.kernel.time.TimeZones
import com.sos.scheduler.engine.main.SchedulerControllerBridge
import java.lang.Thread.currentThread
import javax.annotation.Nullable
import javax.inject.{Inject, Singleton}
import org.joda.time.DateTimeZone.UTC
import scala.collection.JavaConversions._
import scala.util.control.NonFatal
import com.sos.scheduler.engine.kernel.http.{SchedulerHttpResponse, SchedulerHttpRequest}
import com.sos.scheduler.engine.kernel.security.SchedulerSecurityLevel

@ForCpp
@Singleton
final class Scheduler @Inject private(
    cppProxy: SpoolerC,
    controllerBridge: SchedulerControllerBridge,
    prefixLog: PrefixLog,
    disposableCppProxyRegister: DisposableCppProxyRegister,
    pluginSubsystem: PluginSubsystem,
    commandSubsystem: CommandSubsystem,
    databaseSubsystem: DatabaseSubsystem,
    implicit private val schedulerThreadCallQueue: SchedulerThreadCallQueue,
    eventBus: SchedulerEventBus,
    val injector: Injector)
extends Sister
with SchedulerIsClosed
with SchedulerXmlCommandExecutor
with HasInjector {

  private var closed = false
  private var onCloseFunction: Option[() => Unit] = None
  private val callRunner = new CallRunner(schedulerThreadCallQueue.delegate)

  enableJavaUtilLoggingOverSLF4J()
  TimeZones.initialize()
  //DateTimeZone.setDefault(UTC);
  java.util.TimeZone.setDefault(UTC.toTimeZone)       // Für JPA @Temporal(TIMESTAMP), damit Date wirklich UTC enthält. Siehe http://stackoverflow.com/questions/508019
  Thread.currentThread.setContextClassLoader(getClass.getClassLoader)   // Für Mail, http://stackoverflow.com/questions/1969667

  cppProxy.setSister(this)

  if (isStartedByCpp) { // Wenn wir ein controllerBridge haben, ist der Scheduler über Java (CppScheduler.main) aufgerufen worden. Dort wird die Sperre gesetzt.
    threadLock()
    onCloseFunction = Some(threadUnlock _)  //TODO Sperre wird in onClose() zu früh freigegeben, der Scheduler läuft ja noch. Lösung: Start über Java mit CppScheduler.run()
  }

  private def isStartedByCpp =
    controllerBridge eq EmptySchedulerControllerBridge.singleton


  def onCppProxyInvalidated() {}

  @ForCpp private def onClose() {
    closed = true
    try {
      eventBus.publish(new SchedulerCloseEvent)
      eventBus.dispatchEvents()
      try databaseSubsystem.close() catch { case NonFatal(x) => prefixLog.error(s"databaseSubsystem.close(): $x") }
      try pluginSubsystem.close() catch { case NonFatal(x)=> prefixLog.error(s"pluginSubsystem.close(): $x") }
    }
    finally for (o <- onCloseFunction) o()
    eventBus.dispatchEvents()
    disposableCppProxyRegister.tryDisposeAll()
  }

  @ForCpp private def onLoad() {
    pluginSubsystem.initialize()
    controllerBridge.onSchedulerStarted(this)
  }

  @ForCpp private def onActivate() {
    pluginSubsystem.activate()
  }

  @ForCpp private def onActivated() {
    controllerBridge.onSchedulerActivated()
  }

  /** Wird bei jedem Schleifendurchlauf aufgerufen. */
  @ForCpp private def onEnteringSleepState() = {
    eventBus.dispatchEvents()
    val somethingDone = callRunner.executeMatureCalls()
    val nextTime = schedulerThreadCallQueue.nextTime
    if (somethingDone) -nextTime else nextTime
  }

  /** Nur für C++, zur Ausführung eines Kommandos in Java */
  @ForCpp private def javaExecuteXml(xml: String): String = {
    try commandSubsystem.executeXml(xml)
    catch {
      case x: UnknownCommandException => {
        prefixLog.warn(x.toString)
        "UNKNOWN_COMMAND"   // Siehe command_error.cxx, für ordentliche Meldung SCHEDULER-105, bis Java die selbst liefert kann.
      }
    }
  }

  @ForCpp private def getEventSubsystem =
    injector.getInstance(classOf[EventSubsystem])

  @ForCpp private def log(prefix: String, level: Int, line: String) {
    CppLogger.log(prefix, SchedulerLogLevel.ofCpp(level), line)
  }

  @ForCpp private def enqueueCall(o: CppCall) {
    schedulerThreadCallQueue.add(o)
  }

  @ForCpp private def cancelCall(o: CppCall) {
    schedulerThreadCallQueue.tryCancel(o)
  }

  @ForCpp private def threadLock() {
    CppProxy.threadLock.lock()
  }

  @ForCpp /*private*/ def threadUnlock() {
    CppProxy.threadLock.unlock()
  }

  def terminate() {
    try cppProxy.cmd_terminate()
    catch {
      case x: CppProxyInvalidatedException => {
        logger.debug("Scheduler.terminate() ignored because C++ object has already been destroyed", x)
      }
    }
  }

  /** Löst bei einem ERROR-Element eine Exception aus. */
  def executeXml(xml: String): String = {
    val result = uncheckedExecuteXml(xml)
    if (result contains "<ERROR") {
      val doc = loadXml(result)
      for (e <- childElements(doc.getDocumentElement); error <- new NamedChildElements("ERROR", e))
        throw new SchedulerException(error.getAttribute("text"))
    }
    result
  }

  /** execute_xml() der C++-Klasse Spooler */
  def uncheckedExecuteXml(xml: String): String =
    inSchedulerThread { cppProxy.execute_xml(xml) }

  def uncheckedExecuteXml(xml: String, securityLevel: SchedulerSecurityLevel) =
    cppProxy.execute_xml_with_security_level(xml, securityLevel.cppName)

  //    /** @param text Sollte auf \n enden */
  //    public void writeToSchedulerLog(LogCategory category, String text) {
  //        cppProxy.write_to_scheduler_log(category.asString(), text);
  //    }

  def callCppAndDoNothing() {
    cppProxy.tcp_port
  }

  def isClosed = closed
}

@ForCpp
object Scheduler {
  private final val logger = Logger(getClass)

  @ForCpp def of(cppProxy: SpoolerC, @Nullable controllerBridgeOrNull: SchedulerControllerBridge, configurationXml: String) = {
    val controllerBridge = firstNonNull(controllerBridgeOrNull, EmptySchedulerControllerBridge.singleton)
    controllerBridge.getSettings.setSettingsInCpp(cppProxy.modifiable_settings)

    val injector = createInjector(Seq(
      new SchedulerModule(cppProxy, controllerBridge, currentThread()),
      PluginModule(configurationXml)))
    injector.getInstance(classOf[Scheduler])
  }
}
