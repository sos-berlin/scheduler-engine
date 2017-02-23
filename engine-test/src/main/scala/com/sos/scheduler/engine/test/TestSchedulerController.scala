package com.sos.scheduler.engine.test

import com.google.common.base.Splitter
import com.google.common.base.Strings.nullToEmpty
import com.google.common.base.Throwables._
import com.sos.jobscheduler.common.guice.GuiceImplicits._
import com.sos.jobscheduler.common.scalautil.AutoClosing.{autoClosing, closeOnError}
import com.sos.jobscheduler.common.scalautil.{HasCloser, Logger}
import com.sos.jobscheduler.common.time.ScalaTime._
import com.sos.jobscheduler.data.event.KeyedEvent
import com.sos.jobscheduler.data.event.KeyedEvent.NoKey
import com.sos.jobscheduler.data.log.SchedulerLogLevel
import com.sos.jobscheduler.data.message.MessageCode
import com.sos.scheduler.engine.common.xml.CppXmlUtils.{loadXml, prettyXml}
import com.sos.scheduler.engine.data.log.ErrorLogged
import com.sos.scheduler.engine.eventbus._
import com.sos.scheduler.engine.kernel.Scheduler
import com.sos.scheduler.engine.kernel.log.PrefixLog
import com.sos.scheduler.engine.kernel.scheduler.HasInjector
import com.sos.scheduler.engine.kernel.settings.{CppSettingName, CppSettings}
import com.sos.scheduler.engine.kernel.util.Hostware
import com.sos.scheduler.engine.main.{BridgeState, CppBinaries, CppBinary, SchedulerThreadController}
import com.sos.scheduler.engine.test.TestSchedulerController._
import com.sos.scheduler.engine.test.binary.{CppBinariesDebugMode, TestCppBinaries}
import com.sos.scheduler.engine.test.configuration.{HostwareDatabaseConfiguration, JdbcDatabaseConfiguration, TestConfiguration}
import com.sos.scheduler.engine.test.util.IDE.isRunningUnderIDE
import java.io.File
import java.lang.management.ManagementFactory
import java.sql.{Connection, DriverManager}
import java.time.Duration
import org.scalactic.Requirements._
import scala.collection.JavaConversions.iterableAsScalaIterable
import scala.reflect.ClassTag

abstract class TestSchedulerController
extends DelegatingSchedulerController
with HasCloser
with HasInjector {

  def testConfiguration: TestConfiguration

  def environment: TestEnvironment

  private val testName = testConfiguration.testClass.getName
  protected final lazy val delegate = new SchedulerThreadController(testName, cppSettings(testName, testConfiguration, environment.databaseDirectory))
  private val debugMode = testConfiguration.binariesDebugMode getOrElse CppBinariesDebugMode.Debug
  private val logCategories = testConfiguration.logCategories + " " + sys.props.getOrElse("scheduler.logCategories", "").trim
  private var isPrepared: Boolean = false
  private var _scheduler: Scheduler = null
  @volatile private var errorLoggedIsTolerated: ErrorLogged ⇒ Boolean = Set()

  private val jdbcUrlOption: Option[String] =
    testConfiguration.database collect {
      case c: JdbcDatabaseConfiguration =>
        Class forName c.jdbcClassName
        c.testJdbcUrl(testName, environment.databaseDirectory)
    }

  closeOnError(closer) {
    eventBus.onHot[ErrorLogged] { case KeyedEvent(NoKey, e) =>
      // Kann ein anderer Thread sein: C++ Heart_beat_watchdog_thread Abbruchmeldung SCHEDULER-386
      if (testConfiguration.terminateOnError &&
        !(e.codeOption exists testConfiguration.ignoreError) &&
        !errorLoggedIsTolerated(e) &&
        !testConfiguration.errorLoggedIsTolerated(e)) {
        terminateAfterException(new RuntimeException(s"Test terminated after error log message: ${e.message }"))
      }
    }

    eventBus.onHotAndCold[EventHandlerFailedEvent] { case KeyedEvent(NoKey, e) ⇒
      if (testConfiguration.terminateOnError) {
        logger.debug("SchedulerTest is aborted due to 'terminateOnError' and error: " + e)
        terminateAfterException(e.throwable)
      }
    }
  }

  logger.debug("Java VM arguments:" + (ManagementFactory.getRuntimeMXBean.getInputArguments mkString ", "))
  if (isRunningUnderIDE) {
    val possiblePid = ManagementFactory.getRuntimeMXBean.getName takeWhile { _ != '@' }  // http://stackoverflow.com/questions/35842
    logger.debug(s"PID is possibly $possiblePid")
  }

  onClose {
    delegate.close()   // Possibly called twice, then checking for SchedulerClosed handling error
  }

  /** Startet den Scheduler und wartet, bis er aktiv ist. */
  def activateScheduler(args: java.lang.Iterable[String]): Unit = {
    startScheduler(iterableAsScalaIterable(args).toSeq: _*)
    waitUntilSchedulerIsActive()
  }

  /** Startet den Scheduler und wartet, bis er aktiv ist. */
  def activateScheduler(): Unit =
    activateScheduler(Nil: _*)

  /** Startet den Scheduler und wartet, bis er aktiv ist. */
  def activateScheduler(args: String*): Unit = {
    startScheduler(args: _*)
    waitUntilSchedulerIsActive()
  }

  def startScheduler(args: String*): Unit = {
    prepare()
    val extraOptions = nullToEmpty(System.getProperty(classOf[TestSchedulerController].getName + ".options"))
    val allArgs = Seq() ++
        environment.standardArgs(cppBinaries, logCategories = logCategories) ++
        Splitter.on(",").omitEmptyStrings.split(extraOptions) ++
        testConfiguration.mainArguments ++
        args
    delegate.startScheduler(allArgs: _*)
  }

  def prepare(): Unit = {
    if (!isPrepared) {
      environment.prepare()
      delegate.loadModule(cppBinaries.file(CppBinary.moduleFilename))
      isPrepared = true
    }
  }

  def scheduler: Scheduler = {
    requireState(_scheduler != null, "Scheduler is not active yet")
    _scheduler
  }

  /** Wartet, bis das Objekt [[com.sos.scheduler.engine.kernel.Scheduler]] verfügbar ist. */
  def waitUntilSchedulerIsActive(): Unit = {
    val previous = _scheduler
    _scheduler = delegate.waitUntilSchedulerState(BridgeState.active)
    if (_scheduler == null) {
      waitForTermination()   // Should throw the exception causing the activation failure
      throw new RuntimeException("Scheduler aborted before startup")
    }
    if (previous == null && testConfiguration.terminateOnError) checkForErrorLogLine()
  }

  def waitForTermination(): Unit = waitForTermination(TestTimeout)

  def waitForTermination(timeout: Duration): Unit = {
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

  private def checkForErrorLogLine(): Unit = {
    val lastErrorLine = injector.instance[PrefixLog].lastByLevel(SchedulerLogLevel.error)
    if (!lastErrorLine.isEmpty) sys.error("Test terminated after error log line: " + lastErrorLine)
  }

  def suppressingTerminateOnError[A](f: ⇒ A): A =
    toleratingErrorLogged(_ ⇒ true)(f)

  def toleratingErrorCodes[A](tolerateErrorCodes: MessageCode ⇒ Boolean)(f: ⇒ A): A =
    toleratingErrorLogged(_.codeOption exists tolerateErrorCodes)(f)

  def toleratingErrorLogged[A](predicate: ErrorLogged ⇒ Boolean)(f: ⇒ A): A = {
    require(errorLoggedIsTolerated == Set(), "Not nestable")
    errorLoggedIsTolerated = predicate
    try {
      val result = f
      eventBus.dispatchEvents()   // Damit handleEvent(ErrorLogged) wirklich jetzt gerufen wird, solange noch errorLoggedIsTolerated gesetzt ist
      result
    }
    finally errorLoggedIsTolerated = Set()
  }

  final def instance[A : ClassTag]: A = injector.instance[A]

  /** Eine Exception in runnable beendet den Scheduler. */
  def newThread(runnable: Runnable) =
    new Thread {
      override def run(): Unit = {
        try runnable.run()
        catch {
          case t: Throwable =>
            terminateAfterException(t)
            throw propagate(t)
          }
        }
      }

  /** Rechtzeitig aufrufen, dass kein Event verloren geht. */
  def newEventPipe(): EventPipe = new EventPipe(eventBus, TestTimeout)

  def withEventPipe[A](body: EventPipe ⇒ A) = autoClosing(newEventPipe())(body)

  def isStarted =
    delegate.isStarted

  def cppBinaries: CppBinaries =
    TestCppBinaries.cppBinaries(debugMode)

  def newJDBCConnection(): Connection =
    DriverManager.getConnection(jdbcUrlOption.get)
}


object TestSchedulerController {
  /** Long timeout elapses in case of error only. */
  final val TestTimeout = 60.s
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

  object implicits {
    implicit val Timeout = ImplicitTimeout(TestTimeout)
  }
}
