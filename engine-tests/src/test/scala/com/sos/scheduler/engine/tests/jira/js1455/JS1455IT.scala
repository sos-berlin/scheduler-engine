package com.sos.scheduler.engine.tests.jira.js1455

import com.sos.jobscheduler.common.scalautil.FileUtils.implicits._
import com.sos.scheduler.engine.data.job.JobPath
import com.sos.scheduler.engine.test.SchedulerTestUtils._
import com.sos.scheduler.engine.test.configuration.TestConfiguration
import com.sos.scheduler.engine.test.scalatest.ScalaSchedulerTest
import java.nio.file.Files.exists
import org.junit.runner.RunWith
import org.scalatest.FreeSpec
import org.scalatest.junit.JUnitRunner

/**
  * JS-1455 spooler.log.start_new_file should keep log categories, at least "scheduler".
  * <br>
  * JS-1644 Call of spooler_log() api after spooler.log().start_new_file() triggers scheduler.log rotate.
  *
  * @author Joacim Zschimmer
  */
@RunWith(classOf[JUnitRunner])
final class JS1455IT extends FreeSpec with ScalaSchedulerTest {

  override protected lazy val testConfiguration = TestConfiguration(getClass, logCategories = "scheduler.wait")

  private val firstCommand = "<params.get/>"
  private val secondCommand = "<params/>"
  private lazy val schedulerOldLogFile = testEnvironment.logDirectory / "scheduler-old.log"

  "spooler.log contains log category {scheduler}" in {
    assert(numberOfLogStarts == 1)
    scheduler executeXml firstCommand
    assert(schedulerLogAsString contains firstCommand)
    assert(numberOfLogStarts == 1)
    checkSchedulerLog()
  }

  "spooler.log.start_new_file should keep log category {scheduler}" in {
    runJob(JobPath("/start_new_file"))
    scheduler executeXml secondCommand // Logs the command
    withClue("New file should not contain previous lines:") {
      assert(!(schedulerLogAsString contains firstCommand))
    }
    assert(numberOfLogStarts == 1)
    assert(schedulerLogAsString contains secondCommand)
    checkSchedulerLog()
  }

  "JS-1644 spooler_log.info() in next job does not start new scheduler.log" in {
    runJob(JobPath("/log"))
    assert(numberOfLogStarts == 2)
    runJob(JobPath("/log"))
    assert(numberOfLogStarts == 3)
    withClue("After next job with spooler_log.info(), scheduler.log should keep its content: ") {
      assert(schedulerLogAsString contains secondCommand)
    }
  }

  "No scheduler-old.log exists" in {
    assert(!exists(schedulerOldLogFile))
  }

  // For manual testing. To be fast, remove sleep(1) from log.cxx.
  //"Call start_new_files multiple times" in {
  //  runJob(JobPath("/start_new_file_stress"))
  //}

  private def checkSchedulerLog(): Unit = {
    assert(schedulerLogAsString contains "{scheduler}")
    //assert(schedulerLogAsString contains "{scheduler.wait}")
  }

  private def schedulerLogAsString = testEnvironment.schedulerLog.contentString

  private def numberOfLogStarts: Int = schedulerLogAsString split '\n' count { _.indexOf("Aufruf: ") >= 0 }
}
