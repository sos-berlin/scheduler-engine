package com.sos.scheduler.engine.test

import com.sos.scheduler.engine.common.scalautil.AutoClosing.autoClosing
import com.sos.scheduler.engine.test.configuration.TestConfiguration
import org.joda.time.Duration

trait ProvidesTestEnvironment extends ProvidesTestDirectory {

  lazy val testConfiguration =
    TestConfiguration()

  lazy val testEnvironment =
    TestEnvironment(testClass, testConfiguration, testDirectory)

  def newTestSchedulerController() =
    TestSchedulerController(testClass, testConfiguration, testEnvironment)

  def runScheduler[A](activate: Boolean = true)(f: TestSchedulerController => A): A = {
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

object ProvidesTestEnvironment {
  def runScheduler[A](testConfiguration: TestConfiguration = TestConfiguration(), activate: Boolean = true)(f: TestSchedulerController => A): A =
    autoClosing(ProvidesTestEnvironment(testConfiguration)) { env =>
      env.runScheduler(activate = activate)(f)
    }

  def apply(testConfiguration: TestConfiguration = TestConfiguration()) = {
    val conf = testConfiguration
    new ProvidesTestEnvironment {
      override lazy val testConfiguration = conf
    }
  }
}
