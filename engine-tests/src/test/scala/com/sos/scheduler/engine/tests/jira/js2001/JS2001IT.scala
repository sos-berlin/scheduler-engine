package com.sos.scheduler.engine.tests.jira.js2001

import com.sos.scheduler.engine.common.scalautil.Futures.implicits._
import com.sos.scheduler.engine.common.time.ScalaTime._
import com.sos.scheduler.engine.data.scheduler.SchedulerOverview
import com.sos.scheduler.engine.data.scheduler.SchedulerState.{paused, running}
import com.sos.scheduler.engine.kernel.DirectSchedulerClient
import com.sos.scheduler.engine.test.configuration.TestConfiguration
import com.sos.scheduler.engine.test.scalatest.ScalaSchedulerTest
import org.junit.runner.RunWith
import org.scalatest.FreeSpec
import org.scalatest.junit.JUnitRunner

/**
  * JS-2001 "-pause" commandline argument
  */
@RunWith(classOf[JUnitRunner])
final class JS2001IT extends FreeSpec with ScalaSchedulerTest {

  protected override lazy val testConfiguration = TestConfiguration(
    testClass = getClass,
    mainArguments = Seq("-pause"))

  "-pause command line argument" in {
    assert(overview.state == paused)

    scheduler.executeXml(<modify_spooler cmd="continue"/>)
    assert(overview.state == running)
  }

  private def overview: SchedulerOverview =
    instance[DirectSchedulerClient].overview.await(99.s).value
}
