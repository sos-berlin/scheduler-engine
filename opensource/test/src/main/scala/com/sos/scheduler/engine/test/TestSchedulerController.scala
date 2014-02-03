package com.sos.scheduler.engine.test

import TestSchedulerController._
import _root_.scala.Some
import _root_.scala.collection.JavaConversions._
import _root_.scala.reflect.ClassTag
import _root_.scala.sys.error
import com.google.common.base.Splitter
import com.google.common.base.Strings.nullToEmpty
import com.google.common.base.Throwables._
import com.sos.scheduler.engine.common.scalautil.{HasCloser, Logger}
import com.sos.scheduler.engine.common.time.ScalaJoda._
import com.sos.scheduler.engine.common.xml.XmlUtils.{loadXml, prettyXml}
import com.sos.scheduler.engine.data.log.ErrorLogEvent
import com.sos.scheduler.engine.data.log.SchedulerLogLevel
import com.sos.scheduler.engine.eventbus._
import com.sos.scheduler.engine.kernel.Scheduler
import com.sos.scheduler.engine.kernel.log.PrefixLog
import com.sos.scheduler.engine.kernel.scheduler.HasInjector
import com.sos.scheduler.engine.kernel.settings.{CppSettingName, CppSettings}
import com.sos.scheduler.engine.kernel.util.Hostware
import com.sos.scheduler.engine.main.{SchedulerThreadController, CppBinaries, CppBinary, SchedulerState}
import com.sos.scheduler.engine.test.binary.{CppBinariesDebugMode, TestCppBinaries}
import com.sos.scheduler.engine.test.configuration.{HostwareDatabaseConfiguration, JdbcDatabaseConfiguration, TestConfiguration}
import com.sos.scheduler.engine.test.scala.SchedulerTestImplicits._
import java.io.File
import java.sql.{Connection, DriverManager}
import org.joda.time.Duration

abstract class TestSchedulerController
extends DelegatingSchedulerController
with HasCloser
with HasInjector
with EventHandlerAnnotated {

  def testConfiguration: TestConfiguration

  def environment: TestEnvironment

  private val testName = testConfiguration.testClass.getName
  protected final lazy val delegate = new SchedulerThreadController(testName, cppSettings(testName, testConfiguration, environment.databaseDirectory))
  private val eventBus: SchedulerEventBus = getEventBus
  private val thread = Thread.currentThread
  private val debugMode = testConfiguration.binariesDebugMode getOrElse CppBinariesDebugMode.debug
  private val logCategories = testConfiguration.logCategories + " " + sys.props.getOrElse("scheduler.logCategories", "").trim

  private var isPrepared: Boolean = false
  private var _scheduler: Scheduler = null
  @volatile private var suppressTerminateOnError = false

  private val jdbcUrlOption: Option[String] =
    testConfiguration.database collect {
      case c: JdbcDatabaseConfiguration =>
        Class forName c.jdbcClassName
        c.testJdbcUrl(testName, environment.databaseDirectory)
    }

  override def close() {
    try delegate.close()
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
    val allArgs = Seq() ++
        environment.standardArgs(cppBinaries, logCategories) ++
        Splitter.on(",").omitEmptyStrings.split(extraOptions) ++
        testConfiguration.mainArguments ++
        args
    delegate.startScheduler(allArgs)
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
    if (previous == null && testConfiguration.terminateOnError) checkForErrorLogLine()
  }

  def waitForTermination(timeout: Duration) {
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
    require(!suppressTerminateOnError)
    suppressTerminateOnError = true
    try {
      val result = f
      getEventBus.dispatchEvents()   // Damit handleEvent(ErrorLogEvent) wirklich jetzt gerufen wird, solange noch suppressTerminateOnError gilt
      result
    }
    finally suppressTerminateOnError = false
  }

  @EventHandler
  def handleEvent(e: ErrorLogEvent) {
    if (testConfiguration.terminateOnError && !suppressTerminateOnError && !testConfiguration.ignoreError(e.getCodeOrNull) && !testConfiguration.errorLogEventIsExpected(e))
      terminateAfterException(error(s"Test terminated after error log line: ${e.getLine}"))
  }

  @EventHandler @HotEventHandler
  def handleEvent(e: EventHandlerFailedEvent) {
    if (testConfiguration.terminateOnError) {
      logger.debug("SchedulerTest is aborted due to 'terminateOnError' and error: " + e)
      terminateAfterException(e.getThrowable)
    }
  }

  final def instance[A](implicit c: ClassTag[A]): A =
    injector.getInstance(c.runtimeClass.asInstanceOf[Class[A]])

  final def injector =
    scheduler.injector

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
    onClose { eventBus unregisterAnnotated o }
  }

  def isStarted =
    delegate.isStarted

  def cppBinaries: CppBinaries =
    TestCppBinaries.cppBinaries(debugMode)

  def newJDBCConnection(): Connection =
    DriverManager.getConnection(jdbcUrlOption.get)
}


object TestSchedulerController {
  final val shortTimeout = 15.s
  private val logger = Logger(getClass)

  def apply(testConfiguration: TestConfiguration): TestSchedulerController = {
    val _testConfiguration = testConfiguration
    new TestSchedulerController with ProvidesTestEnvironment {
      override lazy val testConfiguration = _testConfiguration
      lazy val environment = testEnvironment
    }
  }

  def apply(testConfiguration: TestConfiguration, testEnvironment: TestEnvironment): TestSchedulerController = {
    val _configuration = testConfiguration
    new TestSchedulerController {
      override lazy val testConfiguration = _configuration
      lazy val environment = testEnvironment
    }
  }

  private def cppSettings(testName: String, configuration: TestConfiguration, databaseDirectory: File): CppSettings =
    CppSettings(
      configuration.cppSettings
        + (CppSettingName.jobJavaClasspath -> System.getProperty("java.class.path"))
        ++ dbNameCppSetting(testName, configuration, databaseDirectory))

  private def dbNameCppSetting(testName: String, configuration: TestConfiguration, databaseDirectory: File): Option[(CppSettingName, String)] =
    configuration.database match {
      case Some(c: JdbcDatabaseConfiguration) =>
        Some(CppSettingName.dbName -> Hostware.databasePath(c.jdbcClassName, c.testJdbcUrl(testName, databaseDirectory)))
      case Some(c: HostwareDatabaseConfiguration) =>
        Some(CppSettingName.dbName -> c.hostwareString)
      case None =>
        None
    }
}
