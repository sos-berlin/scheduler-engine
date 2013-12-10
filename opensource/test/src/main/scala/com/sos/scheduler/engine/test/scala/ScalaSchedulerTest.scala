package com.sos.scheduler.engine.test.scala

import com.sos.scheduler.engine.eventbus.EventHandlerAnnotated
import com.sos.scheduler.engine.test.configuration.TestConfiguration
import com.sos.scheduler.engine.test.scala.Utils._
import com.sos.scheduler.engine.test.{TestSchedulerController, SchedulerTest}
import org.scalatest.{FunSuite, BeforeAndAfterAll}
import scala.reflect.ClassTag

trait ScalaSchedulerTest extends FunSuite with BeforeAndAfterAll with EventHandlerAnnotated {

  protected lazy val testConfiguration = TestConfiguration()
  protected lazy final val controller = new TestSchedulerController(getClass, testConfiguration)

  override protected final def beforeAll() {
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

  /** Wie <code>BeforeAndAfterAll.beforeAll</code>, aber bei einer Exception wird <code>afterAll()</code> aufgerufen. */
  protected def checkedBeforeAll() {
    if (!controller.isStarted) {
      onBeforeSchedulerActivation()
      controller.activateScheduler(testConfiguration.mainArguments: _*)
      onSchedulerActivated()
    }
  }

  override def afterAll() {
    try {
      controller.getEventBus.unregisterAnnotated(this)
      controller.close()
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
