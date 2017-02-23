package com.sos.scheduler.engine.tests.jira.js993

import com.sos.jobscheduler.common.system.Bitness._
import com.sos.scheduler.engine.data.job.JobPath
import com.sos.jobscheduler.data.message.MessageCode
import com.sos.scheduler.engine.test.SchedulerTestUtils.{interceptErrorLogged, runJob}
import com.sos.scheduler.engine.test.scalatest.ScalaSchedulerTest
import org.junit.runner.RunWith
import org.scalatest.FreeSpec
import org.scalatest.junit.JUnitRunner

@RunWith(classOf[JUnitRunner])
final class JS993IT extends FreeSpec with ScalaSchedulerTest {

  for (language ← List("spidermonkey", "javascript")) {
    s"language=$language should work on 32bit and be rejected on 64bit" in {
      val jobPath = JobPath(s"/test-$language")
      bitness match {
        case Bits64 ⇒
          interceptErrorLogged(MessageCode("COM-80020009")) {
            runJob(jobPath)
          }
        case Bits32 ⇒ runJob(jobPath)
      }
    }
  }
}
