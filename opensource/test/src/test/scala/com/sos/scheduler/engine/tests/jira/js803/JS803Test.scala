package com.sos.scheduler.engine.tests.jira.js803

import scala.xml.Elem
import scala.xml.Utility.trim
import org.apache.log4j.Logger
import org.hamcrest.Matchers.equalTo
import org.hamcrest.MatcherAssert.assertThat
import org.joda.time.DateTime
import org.joda.time.format.DateTimeFormat
import org.junit.Assert._
import org.junit.Test
import com.sos.scheduler.engine.kernel.order._
import com.sos.scheduler.engine.kernel.folder.AbsolutePath
import com.sos.scheduler.engine.kernel.util.Time
import com.sos.scheduler.engine.test.scala.SchedulerTestImplicits._
import com.sos.scheduler.engine.eventbus.{HotEventHandler, EventHandler}
import com.sos.scheduler.engine.test.SchedulerTest

/** Ticket JS-803.
 * @see <a href='http://www.sos-berlin.com/jira/browse/JS-803'>JS-803</a>
 * @see com.sos.scheduler.engine.tests.jira.js653.JS653Test */
final class JS803Test extends SchedulerTest {
    import JS803Test._
    @volatile private var startTime = new DateTime(0)
    private var count = 0

    //TODO Manchmal versagt der Test, weil die Aufträge nicht starten. Vielleicht helfen uns die Logzeilen weiter.
    @Test def test() {
        controller.activateScheduler("-e")
        startTime = secondNow() plusSeconds orderDelay
        logger.info(startTime)
        execute(addDailyOrderElem(new OrderKey(jobChainPath, new OrderId("dailyOrder")), startTime))
        execute(addSingleOrderElem(new OrderKey(jobChainPath, new OrderId("singleOrder")), startTime))
        execute(addSingleRuntimeOrderElem(new OrderKey(jobChainPath, new OrderId("singleRuntimeOrder")), startTime))
        controller.waitForTermination(shortTimeout)
    }

    private def execute(command: Elem) {
        logger.info(trim(command))
        controller.scheduler.executeXml(command)
    }

    @EventHandler def handleEvent(e: OrderTouchedEvent) {
        assertTrue("Order "+e.getKey+ " has been started before expected time "+startTime, new DateTime() isAfter startTime)
    }

    @HotEventHandler def handleEvent(event: OrderFinishedEvent, order: UnmodifiableOrder) {
        assertThat("Wrong end state of order "+event.getKey, order.getState, equalTo(expectedEndState))
        count += 1
        if (count == 3)  controller.terminateScheduler()
    }
}

object JS803Test {
    private val logger: Logger = Logger.getLogger(classOf[JS803Test])
    private val shortTimeout = SchedulerTest.shortTimeout
    private val orderDelay = 3+1
    private val jobChainPath = new AbsolutePath("/super")
    private val expectedEndState = new OrderState("state.nestedC.end")
    private val hhmmssFormatter = DateTimeFormat.forPattern("HH:mm:ss")
    private val yyyymmddhhmmssFormatter = DateTimeFormat.forPattern("yyyy-MM-dd HH:mm:ss")

    private def secondNow() = {
        val now = new DateTime()
        now minusMillis now.millisOfSecond.get
    }

    private def addDailyOrderElem(orderKey: OrderKey, startTime: DateTime) =
        <add_order job_chain={orderKey.getJobChainPath.toString} id={orderKey.getId.toString}>
            <run_time>
                <period single_start={hhmmssFormatter.print(startTime)}/>
            </run_time>
        </add_order>

    private def addSingleOrderElem(orderKey: OrderKey, startTime: DateTime) =
        <add_order job_chain={orderKey.getJobChainPath.toString} id={orderKey.getId.toString}
                   at={yyyymmddhhmmssFormatter.print(startTime)}/>

    private def addSingleRuntimeOrderElem(orderKey: OrderKey, startTime: DateTime) =
        <add_order job_chain={orderKey.getJobChainPath.toString} id={orderKey.getId.toString}>
            <run_time>
                <at at={yyyymmddhhmmssFormatter.print(startTime)}/>
            </run_time>
        </add_order>

    private case class TimeEvent(t: Time, e: OrderEvent)
}
