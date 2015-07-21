package com.sos.scheduler.engine.tests.jira.js1455

import com.sos.scheduler.engine.common.scalautil.FileUtils.implicits._
import com.sos.scheduler.engine.data.job.JobPath
import com.sos.scheduler.engine.test.SchedulerTestUtils._
import com.sos.scheduler.engine.test.configuration.TestConfiguration
import com.sos.scheduler.engine.test.scalatest.ScalaSchedulerTest
import org.junit.runner.RunWith
import org.scalatest.FreeSpec
import org.scalatest.junit.JUnitRunner

/**
 * JS-1455 spooler.log.start_new_file should keep log categories, at least "scheduler".
 *
 * @author Joacim Zschimmer
 */
@RunWith(classOf[JUnitRunner])
final class JS1455IT extends FreeSpec with ScalaSchedulerTest {

  override protected lazy val testConfiguration = TestConfiguration(getClass, logCategories = "scheduler.wait")

  "spooler.log contains log category {scheduler}" in {
    checkSchedulerLog()
  }

  "spooler.log.start_new_file should keep log category {scheduler}" in {
    runJobAndWaitForEnd(JobPath("/start_new_file"))
    scheduler executeXml <show_state/>  // Logs the command
    checkSchedulerLog()
  }

  private def checkSchedulerLog(): Unit = {
    assert(schedulerLogAsString contains "{scheduler}")
    assert(schedulerLogAsString contains "{scheduler.wait}")
  }

  private def schedulerLogAsString = testEnvironment.schedulerLog.contentString
}
