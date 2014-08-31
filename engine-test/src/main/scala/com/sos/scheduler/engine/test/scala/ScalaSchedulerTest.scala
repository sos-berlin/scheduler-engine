package com.sos.scheduler.engine.test.scala

import _root_.scala.reflect.ClassTag
import com.sos.scheduler.engine.common.scalautil.HasCloser.implicits._
import com.sos.scheduler.engine.common.scalautil.Logger
import com.sos.scheduler.engine.eventbus.EventHandlerAnnotated
import com.sos.scheduler.engine.test._
import com.sos.scheduler.engine.test.configuration.TestConfiguration
import com.sos.scheduler.engine.test.scala.ScalaSchedulerTest.logger
import com.sos.scheduler.engine.test.scala.Utils.ignoreException
import com.sos.scheduler.engine.test.scalatest.HasCloserBeforeAndAfterAll
import org.scalatest.Suite
import org.joda.time.Duration

trait ScalaSchedulerTest
    extends Suite
    with HasCloserBeforeAndAfterAll
    with EventHandlerAnnotated
    with ProvidesTestDirectory {

  protected def testClass =
    getClass

  protected lazy val testConfiguration =
    TestConfiguration(testClass = getClass)

  protected final lazy val testEnvironment =
    TestEnvironment(testConfiguration, testDirectory)

  protected lazy final val controller =
    TestSchedulerController(testConfiguration, testEnvironment).registerCloseable

  protected implicit final def implicitController =   // Scala 10.3 mag implicit controller nicht, also so
    controller

  override protected final def beforeAll(): Unit = {
    if (testNames.isEmpty) {
      val line = s"EMPTY TEST SUITE ${getClass.getName}"
      logger warn line
      System.err.println(line)
    }
    else
      try {
        controller.getEventBus.registerAnnotated(this)
        onClose { controller.getEventBus.unregisterAnnotated(this) }
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

  /** Zur Bequemlichkeit.
   * @see com.sos.scheduler.engine.test.TestSchedulerController#scheduler(). */
  protected final def scheduler =
    controller.scheduler

  protected implicit def implicitTimeout: ImplicitTimeout = TestSchedulerController.implicits.Timeout

  protected final val TestTimeout: Duration = TestSchedulerController.TestTimeout

  final def injector =
    scheduler.injector

  protected final def instance[A](implicit c: ClassTag[A]): A =
    controller.instance(c)
}

private object ScalaSchedulerTest {
  private val logger = Logger(getClass)
}
