package com.sos.scheduler.engine.test.scala

import org.scalatest.{FunSuite, BeforeAndAfterAll}
import org.scalatest.junit.JUnitRunner
import org.junit.runner.RunWith
import com.sos.scheduler.engine.test.{TestSchedulerController, SchedulerTest}

@RunWith(classOf[JUnitRunner])
trait ScalaSchedulerTest extends FunSuite with BeforeAndAfterAll {
  val controller = TestSchedulerController.of(getClass)

  def shortTimeout = SchedulerTest.shortTimeout   // Zur komfortableren Benutzung

  override def afterAll(configMap: Map[String, Any]) {
    try controller.close()
    finally super.afterAll(configMap)
  }

  /** Zur Bequemlichkeit; dasselbe wie {@link com.sos.scheduler.engine.test.TestSchedulerController#scheduler()}. */
  protected final def scheduler = controller.scheduler
}
