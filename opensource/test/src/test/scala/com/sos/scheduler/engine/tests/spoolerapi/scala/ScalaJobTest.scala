package com.sos.scheduler.engine.tests.spoolerapi.scala

import com.sos.scheduler.engine.test.scala.ScalaSchedulerTest
import com.sos.scheduler.engine.kernel.folder.Path
import com.sos.scheduler.engine.kernel.job.events.TaskEndedEvent
import com.sos.scheduler.engine.eventbus.EventHandler
import org.junit.Test

final class ScalaJobTest extends ScalaSchedulerTest {
    import ScalaJobTest._

    private var countdown = numberOfStarts

    @Test def testScalaJob() {
        controller.setTerminateOnError(false)
        controller.activateScheduler("-e")
        for (i <- 1 to numberOfStarts)  scheduler.executeXml(startJobXml(i).toString())
        controller.waitForTermination(shortTimeout)
    }

    private def startJobXml(index: Int) =
        <start_job job={jobPath.toString}>
            <params>
                <param name="index" value={index.toString}/>
            </params>
        </start_job>

    @EventHandler def handleEvent(e: TaskEndedEvent) {
        if (e.getJobPath == jobPath) {
            countdown -= 1
            if (countdown == 0)
                controller.terminateScheduler()
        }
    }
}

object ScalaJobTest {
    private def jobPath = new Path("/scala")
    private def numberOfStarts = 2
}
