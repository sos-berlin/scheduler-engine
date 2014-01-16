package com.sos.scheduler.engine.test.scala

import com.sos.scheduler.engine.common.scalautil.{HasCloser, Logger}
import com.sos.scheduler.engine.eventbus.EventHandlerAnnotated
import com.sos.scheduler.engine.test.configuration.TestConfiguration
import com.sos.scheduler.engine.test.scala.ScalaSchedulerTest.logger
import com.sos.scheduler.engine.test.scala.Utils.ignoreException
import com.sos.scheduler.engine.test.{StandardTestDirectory, TestSchedulerController, SchedulerTest}
import org.scalatest.{Suite, BeforeAndAfterAll}
import scala.reflect.ClassTag

trait ScalaSchedulerTest
    extends Suite
    with BeforeAndAfterAll
    with EventHandlerAnnotated
    with HasCloser
    with StandardTestDirectory
{

  protected lazy val testName =
    getClass.getName

  protected lazy val testConfiguration =
    TestConfiguration()

  protected lazy final val controller =
    TestSchedulerController(getClass, testConfiguration, testDirectory)

  override protected final def beforeAll() {
    if (testNames.isEmpty) {
      val line = s"EMPTY TEST SUITE ${getClass.getName}"
      logger warn line
      System.err.println(line)
    } else {
      try {
        controller.getEventBus.registerAnnotated(this)
        checkedBeforeAll()
      }
      catch {
        case x: Throwable =>
        ignoreException { afterAll() }
          throw x
      }
    }
  }

  /** Wie <code>BeforeAndAfterAll.beforeAll</code>, aber bei einer Exception wird <code>afterAll()</code> aufgerufen. */
  protected def checkedBeforeAll() {
    if (!controller.isStarted) {
      controller.prepare()
      onBeforeSchedulerActivation()
      controller.activateScheduler(testConfiguration.mainArguments: _*)
      onSchedulerActivated()
    }
  }

  override def afterAll() {
    try {
      controller.getEventBus.unregisterAnnotated(this)
      controller.close()
      close()
    }
    finally super.afterAll()
  }

  protected def onBeforeSchedulerActivation() {}

  protected def onSchedulerActivated() {}

  protected final def instance[A](implicit c: ClassTag[A]) =
    scheduler.injector.getInstance(c.runtimeClass.asInstanceOf[Class[A]])

  /** Zur Bequemlichkeit.
   * @see com.sos.scheduler.engine.test.TestSchedulerController#scheduler(). */
  protected final def scheduler =
    controller.scheduler

  protected final def shortTimeout =
    SchedulerTest.shortTimeout   // Zur komfortableren Benutzung

  protected final def injector =
    scheduler.injector
}

private object ScalaSchedulerTest {
  private val logger = Logger(getClass)
}
