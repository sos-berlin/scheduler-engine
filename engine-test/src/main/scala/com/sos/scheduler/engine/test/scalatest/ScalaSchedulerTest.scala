package com.sos.scheduler.engine.test.scalatest

import com.google.inject.Injector
import com.sos.jobscheduler.common.scalautil.Closers.implicits._
import com.sos.jobscheduler.common.scalautil.Logger
import com.sos.scheduler.engine.eventbus.{EventHandlerAnnotated, SchedulerEventBus}
import com.sos.scheduler.engine.kernel.Scheduler
import com.sos.scheduler.engine.kernel.async.SchedulerThreadCallQueue
import com.sos.scheduler.engine.test._
import com.sos.scheduler.engine.test.configuration.TestConfiguration
import com.sos.scheduler.engine.test.scalatest.ScalaSchedulerTest._
import com.sos.scheduler.engine.test.scalatest.Utils._
import java.time.Duration
import org.scalatest.Suite
import scala.reflect.ClassTag

trait ScalaSchedulerTest
    extends Suite
    with HasCloserBeforeAndAfterAll
    with EventHandlerAnnotated
    with ProvidesTestDirectory {

  protected def testClass = getClass

  protected lazy val testConfiguration = TestConfiguration(testClass = getClass)

  protected final lazy val testEnvironment: TestEnvironment =
    TestEnvironment(testConfiguration, testDirectory).closeWithCloser

  protected implicit lazy final val controller: TestSchedulerController =
    TestSchedulerController(testConfiguration, testEnvironment).closeWithCloser

  protected implicit lazy final val schedulerThreadCallQueue = instance[SchedulerThreadCallQueue]

  override protected final def beforeAll(): Unit = {
    if (testNames.isEmpty) {
      val line = s"EMPTY TEST SUITE ${getClass.getName}"
      logger warn line
      System.err.println(line)
    }
    else
      try {
        eventBus.registerAnnotated(this)
        onClose { eventBus.unregisterAnnotated(this) }
        checkedBeforeAll()
        if (!controller.isStarted) {
          controller.prepare()
          onBeforeSchedulerActivation()
          controller.activateScheduler()
          onSchedulerActivated()
        }
      }
      catch {
        case x: Throwable =>
          ignoreException { afterAll() }
          throw x
      }
  }

  /** Wie <code>BeforeAndAfterAll.beforeAll</code>, aber bei einer Exception wird <code>afterAll()</code> aufgerufen. */
  protected def checkedBeforeAll(): Unit = {}

  protected def onBeforeSchedulerActivation(): Unit = {}

  protected def onSchedulerActivated(): Unit = {}

  protected final def withEventPipe[A](body: EventPipe ⇒ A) = controller.withEventPipe(body)

  protected final def eventBus: SchedulerEventBus = controller.eventBus

  protected implicit def implicitTimeout: ImplicitTimeout = TestSchedulerController.implicits.Timeout

  protected final val TestTimeout: Duration = TestSchedulerController.TestTimeout

  final def injector: Injector = controller.injector

  protected final def instance[A](implicit c: ClassTag[A]): A = controller.instance(c)

  /**
   * For convenience.
   * @see [[com.sos.scheduler.engine.test.TestSchedulerController]]#scheduler().
   */
  protected final def scheduler: Scheduler = controller.scheduler
}

private object ScalaSchedulerTest {
  private val logger = Logger(getClass)
}
