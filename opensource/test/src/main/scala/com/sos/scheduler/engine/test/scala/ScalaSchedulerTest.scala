package com.sos.scheduler.engine.test.scala

import org.scalatest.{FunSuite, BeforeAndAfterAll}
import com.sos.scheduler.engine.test.{TestSchedulerController, SchedulerTest}
import com.sos.scheduler.engine.eventbus.EventHandlerAnnotated
import com.sos.scheduler.engine.test.scala.Utils._

trait ScalaSchedulerTest extends FunSuite with BeforeAndAfterAll with EventHandlerAnnotated {
  val configurationPackage = getClass.getPackage
  lazy val controller = TestSchedulerController.builder(getClass).resourcesPackage(configurationPackage).build

  def shortTimeout = SchedulerTest.shortTimeout   // Zur komfortableren Benutzung
  def injector = scheduler.getInjector

  override protected final def beforeAll(configMap: Map[String, Any]) {
    try {
      controller.getEventBus.registerAnnotated(this);
      checkedBeforeAll(configMap)
    }
    catch {
      case x =>
        ignoreException { afterAll(configMap) }
        throw x
    }
  }

  /** Wie <code>BeforeAndAfterAll.beforeAll</code>, aber bei einer Exception wird <code>afterAll()</code> aufgerufen. */
  protected def checkedBeforeAll() {}

  /** Wie <code>BeforeAndAfterAll.beforeAll</code>, aber bei einer Exception wird <code>afterAll()</code> aufgerufen. */
  protected def checkedBeforeAll(configMap: Map[String, Any]) {
    checkedBeforeAll()
    if (!controller.isStarted)
      controller.activateScheduler()
  }

  override def afterAll(configMap: Map[String, Any]) {
    try {
      controller.getEventBus.unregisterAnnotated(this);
      controller.close()
    }
    finally super.afterAll(configMap)
  }

  /** Zur Bequemlichkeit.
   * @see com.sos.scheduler.engine.test.TestSchedulerController#scheduler(). */
  protected final def scheduler = controller.scheduler
}
