package com.sos.scheduler.engine.test

import com.sos.scheduler.engine.test.configuration.TestConfiguration
import org.joda.time.Duration

trait ProvidesTestEnvironment extends ProvidesTestDirectory {

  lazy val testConfiguration =
    TestConfiguration()

  lazy val testEnvironment =
    TestEnvironment(testClass, testConfiguration, testDirectory)

  def newTestSchedulerController() =
    TestSchedulerController(testClass, testConfiguration, testEnvironment)

  def runScheduler(f: TestSchedulerController => Unit) {
    runScheduler()(f)
  }

  def runScheduler(activate: Boolean = true)(f: TestSchedulerController => Unit) {
    val controller = newTestSchedulerController()
    try {
      if (activate)
        controller.activateScheduler()
      f(controller)
    } finally
      if (controller.isStarted) {
        controller.terminateScheduler()
        try controller.waitForTermination(Duration.standardHours(3))
        finally controller.close()
      }
  }
}
