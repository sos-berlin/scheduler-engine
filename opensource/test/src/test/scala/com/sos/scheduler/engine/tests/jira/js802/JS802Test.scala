package com.sos.scheduler.engine.tests.jira.js802

import org.joda.time.DateTime
import org.joda.time.format.DateTimeFormat
import org.junit.Assert._
import org.junit.Test
import com.sos.scheduler.engine.kernel.folder.AbsolutePath
import com.sos.scheduler.engine.kernel.order._
import com.sos.scheduler.engine.test.scala.ScalaSchedulerTest
import com.sos.scheduler.engine.test.scala.SchedulerTestImplicits._
import com.sos.scheduler.engine.kernel.job.events.TaskEndedEvent
import scala.collection.mutable
import com.sos.scheduler.engine.eventbus.EventHandler

/** JS-802 "http://www.sos-berlin.com/jira/browse/JS-802": Testet einen Auftrag und einen Job.
 * @see <a href="http://www.sos-berlin.com/jira/browse/JS-802">JS-802</a>*/
class JS802Test extends ScalaSchedulerTest {
    import JS802Test._
    @volatile private var startTime = new DateTime(0)

    private val collector = new {
        val set = mutable.HashSet[Any]()

        def add(o: Any) {
            assertFalse("Duplicate event " +o, set contains o)
            set.add(o)
            if (set == Set(orderKey, jobName))
                controller.terminateScheduler()
        }
    }

    @Test def test() {
        controller.activateScheduler("-e")
        startTime = secondNow() plusSeconds orderDelay
        scheduler.executeXml(orderElem(orderKey, startTime))
        scheduler.executeXml(jobElem(jobName, startTime))
        controller.waitForTermination(shortTimeout)
    }

    @EventHandler def handleEvent(event: OrderTouchedEvent) {
        if (event.getKey == orderKey) {
            assertTrue("Order "+event.getKey+ " has been started before expected time "+startTime, new DateTime() isAfter startTime)
            collector.add(event.getKey)
        }
    }

    @EventHandler def handleEvent(event: TaskEndedEvent) {
        assertTrue("Job "+event.getJobPath+ " has been started before expected time "+startTime, new DateTime() isAfter startTime)
        if (event.getJobPath.getName == jobName)
            collector.add(event.getJobPath.getName)
    }
}

object JS802Test {
    private val orderDelay = 3+1
    private val orderKey = new OrderKey(new AbsolutePath("/a"), new OrderId("atOrder"))
    private val jobName = "job"
    private val yyyymmddhhmmssFormatter = DateTimeFormat.forPattern("yyyy-MM-dd HH:mm:ss")

    private def secondNow() = {
        val now = new DateTime()
        now minusMillis now.millisOfSecond.get
    }

    private def orderElem(orderKey: OrderKey, startTime: DateTime) =
        <order job_chain={orderKey.getJobChainPath.toString} id={orderKey.getId.toString}>
            <run_time>
                <at at={yyyymmddhhmmssFormatter.print(startTime)}/>
            </run_time>
        </order>

    private def jobElem(jobName: String, startTime: DateTime) =
        <job name={jobName}>
            <script java_class={classOf[com.sos.scheduler.engine.test.jobs.SingleStepJob].getName}/>
            <run_time>
                <at at={yyyymmddhhmmssFormatter.print(startTime)}/>
            </run_time>
        </job>
}
