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

  private val testCommand = "<params.get/>"

  "spooler.log contains log category {scheduler}" in {
    scheduler executeXml testCommand
    assert(schedulerLogAsString contains testCommand)
    checkSchedulerLog()
  }

  "spooler.log.start_new_file should keep log category {scheduler}" in {
    runJob(JobPath("/start_new_file"))
    scheduler executeXml <params/>  // Logs the command
    withClue("New file should not contain previous lines:") {
      assert(!(schedulerLogAsString contains testCommand))
    }
    checkSchedulerLog()
  }

  // For manual testing. To be fast, remove sleep(1) from log.cxx.
  //"Call start_new_files multiple times" in {
  //  runJob(JobPath("/start_new_file_stress"))
  //}

  private def checkSchedulerLog(): Unit = {
    assert(schedulerLogAsString contains "{scheduler}")
    assert(schedulerLogAsString contains "{scheduler.wait}")
  }

  private def schedulerLogAsString = testEnvironment.schedulerLog.contentString
}
