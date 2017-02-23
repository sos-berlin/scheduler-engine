package com.sos.scheduler.engine.tests.scheduler.job.temporary

import com.sos.jobscheduler.common.time.ScalaTime._
import com.sos.jobscheduler.common.time.WaitForCondition.waitForCondition
import com.sos.scheduler.engine.data.filebased.FileBasedRemoved
import com.sos.scheduler.engine.data.job.{JobPath, TaskEnded}
import com.sos.scheduler.engine.kernel.job.JobSubsystemClient
import com.sos.scheduler.engine.test.scalatest.ScalaSchedulerTest
import org.junit.runner.RunWith
import org.scalatest.FreeSpec
import org.scalatest.junit.JUnitRunner

@RunWith(classOf[JUnitRunner])
final class TemporaryJobIT extends FreeSpec with ScalaSchedulerTest {
  "Temporary job" in {
    val eventPipe = controller.newEventPipe()
    val jobPath = JobPath("/TEMP")
    scheduler executeXml <job name={jobPath.name} temporary="yes"><script language="shell">exit 0</script><run_time once="yes"/></job>
    eventPipe.nextWhen[TaskEnded]( _.key.jobPath == jobPath )
    eventPipe.next[FileBasedRemoved.type](jobPath)
    waitForCondition(3.s, 100.ms) { !instance[JobSubsystemClient].contains(jobPath) || fail("Temporary job has not been removed") }
  }
}
