package com.sos.scheduler.engine.test

import TestSchedulerController._
import _root_.scala.collection.JavaConversions._
import _root_.scala.collection.mutable
import _root_.scala.sys.error
import com.google.common.base.Splitter
import com.google.common.base.Strings.nullToEmpty
import com.google.common.base.Throwables._
import com.sos.scheduler.engine.common.scalautil.Logger
import com.sos.scheduler.engine.common.system.Files
import com.sos.scheduler.engine.common.system.Files.makeTemporaryDirectory
import com.sos.scheduler.engine.common.system.Files.tryRemoveDirectoryRecursivly
import com.sos.scheduler.engine.common.time.ScalaJoda._
import com.sos.scheduler.engine.common.time.Time
import com.sos.scheduler.engine.common.utils.SosAutoCloseable
import com.sos.scheduler.engine.common.xml.XmlUtils.prettyXml
import com.sos.scheduler.engine.data.log.ErrorLogEvent
import com.sos.scheduler.engine.data.log.SchedulerLogLevel
import com.sos.scheduler.engine.eventbus._
import com.sos.scheduler.engine.kernel.Scheduler
import com.sos.scheduler.engine.kernel.log.PrefixLog
import com.sos.scheduler.engine.kernel.settings.SettingName
import com.sos.scheduler.engine.kernel.settings.Settings
import com.sos.scheduler.engine.kernel.util.Hostware
import com.sos.scheduler.engine.kernel.util.ResourcePath
import com.sos.scheduler.engine.main.CppBinaries
import com.sos.scheduler.engine.main.CppBinary
import com.sos.scheduler.engine.main.SchedulerState
import com.sos.scheduler.engine.test.binary.CppBinariesDebugMode
import com.sos.scheduler.engine.test.binary.TestCppBinaries
import com.sos.scheduler.engine.test.scala.SchedulerTestImplicits._
import java.io.File
import java.sql.Connection
import java.sql.DriverManager
import org.joda.time.Duration
import com.sos.scheduler.engine.common.scalautil.SideEffect._

final class TestSchedulerController(
    testClass: Class[_],
    configuration: TestConfiguration)
extends DelegatingSchedulerController(testClass.getName)
with EventHandlerAnnotated with SosAutoCloseable {

  private val testName: String = testClass.getName

  private val eventBus: SchedulerEventBus = getEventBus
  private val thread = Thread.currentThread
  lazy val environment = new Environment(
    resourcePath = new ResourcePath(configuration.testPackage getOrElse testClass.getPackage),
    directory = workDirectory(testClass),
    nameMap = configuration.resourceNameMap.toMap,
    fileTransformer = configuration.resourceToFileTransformer getOrElse StandardResourceToFileTransformer.singleton)

  private val debugMode = configuration.binariesDebugMode getOrElse CppBinariesDebugMode.debug
  private val jdbcUrl = "jdbc:h2:mem:scheduler-" + testName
  private var terminateOnError: Boolean = true
  private var isPrepared: Boolean = false
  private var logCategories = configuration.logCategories
  private var _scheduler: Scheduler = null
  private val closingRunnables = mutable.Buffer[() => Unit]()

  setSettings(Settings.of(SettingName.jobJavaClasspath, System.getProperty("java.class.path")))
  if (configuration.database.use) useDatabase(configuration.database.closeDelay)

  private def workDirectory(testClass: Class[_]): File = {
    System.getProperty(workDirectoryPropertyName) match {
      case null =>
        makeTemporaryDirectory() sideEffect { o =>
          closingRunnables += { () => tryRemoveDirectoryRecursivly(o) }
        }
      case workDir =>
        new File(workDir).mkdir
        new File(workDir, testName) sideEffect makeCleanDirectory
    }
  }

  def close() {
    try getDelegate.close()
    finally for (r <- closingRunnables.view.reverse) r()
  }

  /** Bricht den Test mit Fehler ab, wenn ein [[com.sos.scheduler.engine.data.log.ErrorLogEvent]] ausgelöst worden ist. */
  def setTerminateOnError(o: Boolean) {
    getDelegate.checkIsNotStarted()
    terminateOnError = o
  }

  def setLogCategories(o: String) {
    getDelegate.checkIsNotStarted()
    logCategories = o
  }

  /** Startet den Scheduler und wartet, bis er aktiv ist. */
  def activateScheduler(args: java.lang.Iterable[String]) {
    startScheduler(iterableAsScalaIterable(args).toSeq: _*)
    waitUntilSchedulerIsActive()
  }

  /** Startet den Scheduler und wartet, bis er aktiv ist. */
  def activateScheduler() {
    activateScheduler(Nil: _*)
  }

  /** Startet den Scheduler und wartet, bis er aktiv ist. */
  def activateScheduler(args: String*) {
    startScheduler(args: _*)
    waitUntilSchedulerIsActive()
  }

  def startScheduler(args: java.lang.Iterable[String]) {
    startScheduler(iterableAsScalaIterable(args).toSeq)
  }

  def startScheduler(args: String*) {
    prepare()
    val extraOptions = nullToEmpty(System.getProperty(classOf[TestSchedulerController].getName + ".options"))
    val allArgs = environment.standardArgs(cppBinaries, logCategories) ++ Splitter.on(",").omitEmptyStrings.split(extraOptions) ++ args
    getDelegate.startScheduler(Seq() ++ allArgs)
  }

  def prepare() {
    if (!isPrepared) {
      registerEventHandler(this)
      environment.prepare()
      getDelegate.loadModule(cppBinaries.file(CppBinary.moduleFilename))
      isPrepared = true
    }
  }

  def scheduler: Scheduler = {
    if (_scheduler == null) {
      automaticStart()
      waitUntilSchedulerIsActive()
    }
    _scheduler
  }

  /** Wartet, bis das Objekt [[com.sos.scheduler.engine.kernel.Scheduler]] verfügbar ist. */
  def waitUntilSchedulerIsActive() {
    val previous = _scheduler
    _scheduler = getDelegate.waitUntilSchedulerState(SchedulerState.active)
    if (_scheduler == null) throw new RuntimeException("Scheduler aborted before startup")
    if (previous == null && terminateOnError) checkForErrorLogLine()
  }

  def waitForTermination(timeout: Time) {
    val ok = tryWaitForTermination(timeout)
    if (!ok) {
      val x = new SchedulerRunningAfterTimeoutException(timeout)
      logger warn x.toString
      val cmd = "<show_state what='folders jobs job_params job_commands tasks task_queue job_chains orders remote_schedulers operations'/>"
      logger warn cmd
      logger warn prettyXml(scheduler.uncheckedExecuteXml(cmd))
      throw x
    }
  }

  private def automaticStart() {
    if (!getDelegate.isStarted) {
      if (Thread.currentThread ne thread)  throw new IllegalStateException("TestSchedulerController.automaticStart() must be called in constructing thread")
      error("JobScheduler is not active yet")
    }
  }

  private def checkForErrorLogLine() {
    val lastErrorLine = _scheduler.instance[PrefixLog].lastByLevel(SchedulerLogLevel.error)
    if (!lastErrorLine.isEmpty) error("Test terminated after error log line: " + lastErrorLine)
  }

  @EventHandler
  def handleEvent(e: ErrorLogEvent) {
    if (!configuration.expectedErrorLogEventPredicate(e) && terminateOnError)
      terminateAfterException(error(s"Test terminated after error log line: ${e.getLine}"))
  }

  @EventHandler @HotEventHandler
  def handleEvent(e: EventHandlerFailedEvent) {
    if (terminateOnError) {
      logger.debug("SchedulerTest is aborted due to 'terminateOnError' and error: " + e)
      terminateAfterException(e.getThrowable)
    }
  }

  /** Eine Exception in runnable beendet den Scheduler. */
  def newThread(runnable: Runnable) =
    new Thread {
      override def run() {
        try runnable.run()
        catch {
          case t: Throwable => {
            terminateAfterException(t)
            throw propagate(t)
          }
        }
      }
    }

  /** Rechtzeitig aufrufen, dass kein Event verloren geht. */
  def newEventPipe(): EventPipe = {
    val result = new EventPipe(eventBus, shortTimeout.toDuration)
    registerEventHandler(result)
    result
  }

  private def registerEventHandler(o: EventHandlerAnnotated) {
    eventBus registerAnnotated o
    closingRunnables += { () =>  eventBus.unregisterAnnotated(o) }
  }

  def isStarted =
    getDelegate.isStarted

  def useDatabase() {
    useDatabase(closeDelay = 0.s)
  }

  def useDatabase(closeDelay: Duration) {
    val suffix = if (closeDelay == Duration.ZERO) "" else ";DB_CLOSE_DELAY=" + closeDelay.plus(999).getStandardSeconds
    val dbName = Hostware.databasePath(jdbcClass, jdbcUrl + suffix)
    setSettings(Settings.of(SettingName.dbName, dbName))
  }

  def cppBinaries: CppBinaries =
    TestCppBinaries.cppBinaries(debugMode)

  def newJDBCConnection(): Connection = {
    Class.forName(jdbcClass)
    DriverManager.getConnection(jdbcUrl)
  }
}

object TestSchedulerController {
  final val shortTimeout = Time.of(15.0)
  private val logger = Logger(getClass)
  private val workDirectoryPropertyName = "com.sos.scheduler.engine.test.directory"
  private val jdbcClass = "org.h2.Driver"

  private def makeCleanDirectory(directory: File) {
    Files.makeDirectory(directory)
    Files.removeDirectoryContentRecursivly(directory)
  }
}
