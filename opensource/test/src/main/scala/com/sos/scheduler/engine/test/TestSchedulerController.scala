package com.sos.scheduler.engine.test

import TestSchedulerController._
import _root_.scala.collection.JavaConversions._
import _root_.scala.collection.mutable
import _root_.scala.sys.error
import com.google.common.base.Splitter
import com.google.common.base.Strings.nullToEmpty
import com.google.common.base.Throwables._
import com.sos.scheduler.engine.common.scalautil.{HasCloser, Logger}
import com.sos.scheduler.engine.common.time.Time
import com.sos.scheduler.engine.common.utils.SosAutoCloseable
import com.sos.scheduler.engine.common.xml.XmlUtils.{loadXml, prettyXml}
import com.sos.scheduler.engine.data.log.ErrorLogEvent
import com.sos.scheduler.engine.data.log.SchedulerLogLevel
import com.sos.scheduler.engine.eventbus._
import com.sos.scheduler.engine.kernel.Scheduler
import com.sos.scheduler.engine.kernel.log.PrefixLog
import com.sos.scheduler.engine.kernel.settings.{CppSettingName, CppSettings}
import com.sos.scheduler.engine.kernel.util.{Hostware, ResourcePath}
import com.sos.scheduler.engine.main.{SchedulerThreadController, CppBinaries, CppBinary, SchedulerState}
import com.sos.scheduler.engine.test.binary.CppBinariesDebugMode
import com.sos.scheduler.engine.test.binary.TestCppBinaries
import com.sos.scheduler.engine.test.configuration.{HostwareDatabaseConfiguration, JdbcDatabaseConfiguration, TestConfiguration}
import com.sos.scheduler.engine.test.scala.SchedulerTestImplicits._
import java.io.File
import java.sql.Connection
import java.sql.DriverManager

abstract class TestSchedulerController
extends DelegatingSchedulerController
with EventHandlerAnnotated
with SosAutoCloseable
with HasCloser {

  def testClass: Class[_]

  def configuration: TestConfiguration

  def testDirectory: File


  final val testName = testClass.getName
  protected final val delegate = new SchedulerThreadController(testName, cppSettings(testName, configuration))
  private val eventBus: SchedulerEventBus = getEventBus
  private val thread = Thread.currentThread

  lazy val environment = new TestEnvironment(
    resourcePath = new ResourcePath(configuration.testPackage getOrElse testClass.getPackage),
    directory = testDirectory,
    nameMap = configuration.resourceNameMap.toMap,
    fileTransformer = configuration.resourceToFileTransformer getOrElse StandardResourceToFileTransformer.singleton)

  private val debugMode = configuration.binariesDebugMode getOrElse CppBinariesDebugMode.debug
  private val logCategories = configuration.logCategories + " " + sys.props.getOrElse("scheduler.logCategories", "").trim

  private var isPrepared: Boolean = false
  private var _scheduler: Scheduler = null
  private val closingRunnables = mutable.Buffer[() => Unit]()
  private var suppressTerminatedOnError = false

  private val jdbcUrlOption: Option[String] =
    configuration.database collect {
      case c: JdbcDatabaseConfiguration =>
        Class forName c.jdbcClassName
        c.testJdbcUrl(testName)
    }

  override def close() {
    try delegate.close()
    finally for (r <- closingRunnables.view.reverse) r()
    super.close()
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
    delegate.startScheduler(Seq() ++ allArgs)
  }

  def prepare() {
    if (!isPrepared) {
      registerEventHandler(this)
      environment.prepare()
      delegate.loadModule(cppBinaries.file(CppBinary.moduleFilename))
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
    _scheduler = delegate.waitUntilSchedulerState(SchedulerState.active)
    if (_scheduler == null) throw new RuntimeException("Scheduler aborted before startup")
    if (previous == null && configuration.terminateOnError) checkForErrorLogLine()
  }

  def waitForTermination(timeout: Time) {
    val ok = tryWaitForTermination(timeout)
    if (!ok) {
      val x = new SchedulerRunningAfterTimeoutException(timeout)
      logger warn x.toString
      val cmd = "<show_state what='folders jobs job_params job_commands tasks task_queue job_chains orders remote_schedulers operations'/>"
      logger warn cmd
      logger warn prettyXml(loadXml(scheduler.uncheckedExecuteXml(cmd)))
      throw x
    }
  }

  private def automaticStart() {
    if (!delegate.isStarted) {
      if (Thread.currentThread ne thread)  throw new IllegalStateException("TestSchedulerController.automaticStart() must be called in constructing thread")
      error("JobScheduler is not active yet")
    }
  }

  private def checkForErrorLogLine() {
    val lastErrorLine = _scheduler.instance[PrefixLog].lastByLevel(SchedulerLogLevel.error)
    if (!lastErrorLine.isEmpty) error("Test terminated after error log line: " + lastErrorLine)
  }

  def suppressingTerminateOnError[A](f: => A): A = {
    require(!suppressTerminatedOnError)
    suppressTerminatedOnError = true
    try f
    finally suppressTerminatedOnError = false
  }

  @EventHandler
  def handleEvent(e: ErrorLogEvent) {
    if (configuration.terminateOnError && !suppressTerminatedOnError && !configuration.ignoreError(e.getCodeOrNull) && !configuration.errorLogEventIsExpected(e))
      terminateAfterException(error(s"Test terminated after error log line: ${e.getLine}"))
  }

  @EventHandler @HotEventHandler
  def handleEvent(e: EventHandlerFailedEvent) {
    if (configuration.terminateOnError) {
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
          case t: Throwable =>
            terminateAfterException(t)
            throw propagate(t)
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
    delegate.isStarted

  def cppBinaries: CppBinaries =
    TestCppBinaries.cppBinaries(debugMode)

  def newJDBCConnection(): Connection =
    DriverManager.getConnection(jdbcUrlOption.get)
}

object TestSchedulerController {
  final val shortTimeout = Time.of(15.0)
  private val logger = Logger(getClass)

  def apply(testClass: Class[_]): TestSchedulerController =
    apply(testClass, TestConfiguration())

  def apply(testClass: Class[_], configuration: TestConfiguration): TestSchedulerController = {
    val _testClass = testClass
    val _configuration = configuration

    new TestSchedulerController with StandardTestDirectory {
      def testClass = _testClass
      def configuration = _configuration
    }
  }

  def apply(testClass: Class[_], configuration: TestConfiguration, testDirectory: File): TestSchedulerController = {
    val _testClass = testClass
    val _configuration = configuration
    val _testDirectory = testDirectory

    new TestSchedulerController  {
      def testClass = _testClass
      def configuration = _configuration
      def testDirectory = _testDirectory
    }
  }

  private def cppSettings(testName: String, configuration: TestConfiguration): CppSettings =
    CppSettings(
      configuration.cppSettings
        + (CppSettingName.jobJavaClasspath -> System.getProperty("java.class.path"))
        ++ dbNameCppSetting(testName, configuration))

  private def dbNameCppSetting(testName: String, configuration: TestConfiguration): Option[(CppSettingName, String)] =
    configuration.database match {
      case Some(c: JdbcDatabaseConfiguration) =>
        Some(CppSettingName.dbName -> Hostware.databasePath(c.jdbcClassName, c.testJdbcUrl(testName)))
      case Some(c: HostwareDatabaseConfiguration) =>
        Some(CppSettingName.dbName -> c.hostwareString)
      case None =>
        None
    }
}
