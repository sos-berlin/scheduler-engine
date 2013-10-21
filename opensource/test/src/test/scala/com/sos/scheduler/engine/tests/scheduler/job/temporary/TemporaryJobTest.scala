package com.sos.scheduler.engine.tests.scheduler.job.temporary

import com.sos.scheduler.engine.data.folder.{FileBasedRemovedEvent, JobPath}
import com.sos.scheduler.engine.data.job.TaskEndedEvent
import com.sos.scheduler.engine.kernel.job.JobSubsystem
import com.sos.scheduler.engine.test.scala.ScalaSchedulerTest
import com.sos.scheduler.engine.test.scala.SchedulerTestImplicits._
import org.junit.runner.RunWith
import org.scalatest.junit.JUnitRunner
import com.sos.scheduler.engine.test.util.time.WaitForCondition.waitForCondition
import com.sos.scheduler.engine.common.time.ScalaJoda._
import scala.util.Try

@RunWith(classOf[JUnitRunner])
class TemporaryJobTest extends ScalaSchedulerTest {
  test("Temporary job") {
    val eventPipe = controller.newEventPipe()
    val jobPath = JobPath.of("/TEMP")
    scheduler executeXml <job name={jobPath.getName} temporary="yes"><script language="shell">exit 0</script><run_time once="yes"/></job>
    eventPipe.nextWithCondition[TaskEndedEvent]( _.jobPath == jobPath )
    eventPipe.nextKeyed[FileBasedRemovedEvent](jobPath)
    waitForCondition(3.s, 100.ms) { Try(instance[JobSubsystem].job(jobPath)).isFailure } || fail("Temporary job has not been removed")
  }
}
