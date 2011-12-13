package com.sos.scheduler.engine.tests.spoolerapi.scala

import org.junit.Test
import com.sos.scheduler.engine.kernel.folder.AbsolutePath
import com.sos.scheduler.engine.kernel.job.events.TaskEndedEvent
import com.sos.scheduler.engine.test.scala._
import com.sos.scheduler.engine.test.scala.SchedulerTestImplicits._
import com.sos.scheduler.engine.kernel.log.SchedulerLogLevel

final class ScalaJobTest extends ScalaSchedulerTest {
    import ScalaJobTest._
    private val eventPipe = controller.newEventPipe()

    controller.setTerminateOnError(false)
    controller.activateScheduler("-e")

    @Test def testScalaJob() {
        runJob(SchedulerLogLevel.info)
        runJob(SchedulerLogLevel.error)
    }

    private def runJob(logLevel: SchedulerLogLevel) {
        scheduler.executeXml(startJobElem(logLevel))
        eventPipe.expectEvent(shortTimeout){e: TaskEndedEvent => e.getJobPath == jobPath}
    }

    private def startJobElem(logLevel: SchedulerLogLevel) =
        <start_job job={jobPath.toString}>
            <params>
                <param name="logLevel" value={logLevel.getNumber.toString}/>
            </params>
        </start_job>
}

object ScalaJobTest {
    private def jobPath = new AbsolutePath("/scala")
}
