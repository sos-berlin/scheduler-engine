package com.sos.scheduler.engine.tests.scheduler.job.temporary

import com.sos.scheduler.engine.common.time.ScalaJoda._
import com.sos.scheduler.engine.common.time.WaitForCondition
import com.sos.scheduler.engine.data.filebased.FileBasedRemovedEvent
import com.sos.scheduler.engine.data.job.{JobPath, TaskEndedEvent}
import com.sos.scheduler.engine.kernel.job.JobSubsystem
import com.sos.scheduler.engine.test.scalatest.ScalaSchedulerTest
import WaitForCondition.waitForCondition
import org.junit.runner.RunWith
import org.scalatest.FreeSpec
import org.scalatest.junit.JUnitRunner
import scala.util.Try

@RunWith(classOf[JUnitRunner])
final class TemporaryJobIT extends FreeSpec with ScalaSchedulerTest {
  "Temporary job" in {
    val eventPipe = controller.newEventPipe()
    val jobPath = JobPath("/TEMP")
    scheduler executeXml <job name={jobPath.name} temporary="yes"><script language="shell">exit 0</script><run_time once="yes"/></job>
    eventPipe.nextWithCondition[TaskEndedEvent]( _.jobPath == jobPath )
    eventPipe.nextKeyed[FileBasedRemovedEvent](jobPath)
    waitForCondition(3.s, 100.ms) { Try(instance[JobSubsystem].job(jobPath)).isFailure } || fail("Temporary job has not been removed")
  }
}
