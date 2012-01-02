package com.sos.scheduler.engine.test.scala

import org.scalatest.{FunSuite, BeforeAndAfterAll}
import com.sos.scheduler.engine.test.{TestSchedulerController, SchedulerTest}

trait ScalaSchedulerTest extends FunSuite with BeforeAndAfterAll with CheckedBeforeAll {
  val controller = TestSchedulerController.of(getClass)

  def shortTimeout = SchedulerTest.shortTimeout   // Zur komfortableren Benutzung

  override def afterAll(configMap: Map[String, Any]) {
    try controller.close()
    finally super.afterAll(configMap)
  }

  /** Zur Bequemlichkeit
   * @see com.sos.scheduler.engine.test.TestSchedulerController#scheduler(). */
  protected final def scheduler = controller.scheduler
}
