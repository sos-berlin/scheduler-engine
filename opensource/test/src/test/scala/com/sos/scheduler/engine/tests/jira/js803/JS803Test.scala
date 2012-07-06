package com.sos.scheduler.engine.tests.jira.js803

import com.sos.scheduler.engine.data.folder.JobChainPath
import com.sos.scheduler.engine.data.order._
import com.sos.scheduler.engine.eventbus.{HotEventHandler, EventHandler}
import com.sos.scheduler.engine.kernel.order._
import com.sos.scheduler.engine.test.SchedulerTest
import com.sos.scheduler.engine.test.scala.SchedulerTestImplicits._
import org.apache.log4j.Logger
import org.hamcrest.MatcherAssert.assertThat
import org.hamcrest.Matchers.equalTo
import org.joda.time.DateTime
import org.joda.time.format.DateTimeFormat
import org.junit.Assert._
import org.junit.Test
import scala.collection.mutable
import scala.xml.Elem
import scala.xml.Utility.trim

/** Ticket JS-803.
 * @see <a href='http://www.sos-berlin.com/jira/browse/JS-803'>JS-803</a>
 * @see com.sos.scheduler.engine.tests.jira.js653.JS653Test */
final class JS803Test extends SchedulerTest {

  import JS803Test._

  private val expectedOrders = new mutable.HashSet[OrderId]
  private val terminatedOrders = new mutable.HashSet[OrderId]
  private var startTime: DateTime = null

  @Test def test() {
    controller.activateScheduler()
    startTime = secondNow() plusSeconds orderDelay
    addOrder(new OrderKey(jobChainPath, new OrderId("dailyOrder")), addDailyOrderElem)
    addOrder(new OrderKey(jobChainPath, new OrderId("singleOrder")), addSingleOrderElem)
    addOrder(new OrderKey(jobChainPath, new OrderId("singleRuntimeOrder")), addSingleRuntimeOrderElem)
    try controller.waitForTermination(shortTimeout)
    finally (expectedOrders diff terminatedOrders).toList match {
      case List() =>
      case notTerminatedOrders => logger.error("Orders failed to terminate: "+ (notTerminatedOrders mkString ", "))
    }
  }

  private def addOrder(orderKey: OrderKey, orderElemFunction: (OrderKey, DateTime) => Elem) {
    execute(orderElemFunction(orderKey, startTime))
    expectedOrders.add(orderKey.getId)
  }

  private def execute(command: Elem) {
    logger.debug(trim(command))
    controller.scheduler.executeXml(command)
  }

  @EventHandler def handleEvent(e: OrderTouchedEvent) {
    assertTrue("Order "+e.getKey+ " has been started before expected time "+startTime, new DateTime() isAfter startTime)
  }

  @HotEventHandler def handleHotEvent(event: OrderFinishedEvent, order: UnmodifiableOrder) {
    assertThat("Wrong end state of order "+event.getKey, order.getState, equalTo(expectedEndState))
  }

  @EventHandler def handleEvent(event: OrderFinishedEvent) {
    terminatedOrders.add(event.getKey.getId)
    if (terminatedOrders == expectedOrders)  controller.terminateScheduler()
  }
}

object JS803Test {
  private val logger = Logger.getLogger(classOf[JS803Test])
  private val shortTimeout = SchedulerTest.shortTimeout
  private val orderDelay = 3+1
  private val jobChainPath = JobChainPath.of("/super")
  private val expectedEndState = new OrderState("state.nestedC.end")
  private val hhmmssFormatter = DateTimeFormat.forPattern("HH:mm:ss")
  private val yyyymmddhhmmssFormatter = DateTimeFormat.forPattern("yyyy-MM-dd HH:mm:ss")

  private def secondNow() = {
    val now = new DateTime()
    now minusMillis now.millisOfSecond.get
  }

  private def addDailyOrderElem(orderKey: OrderKey, startTime: DateTime) =
    <add_order job_chain={orderKey.jobChainPathString} id={orderKey.idString}>
      <run_time>
        <period single_start={hhmmssFormatter.print(startTime)}/>
      </run_time>
    </add_order>

  private def addSingleOrderElem(orderKey: OrderKey, startTime: DateTime) =
    <add_order job_chain={orderKey.jobChainPathString} id={orderKey.idString}
               at={yyyymmddhhmmssFormatter.print(startTime)}/>

  private def addSingleRuntimeOrderElem(orderKey: OrderKey, startTime: DateTime) =
    <add_order job_chain={orderKey.jobChainPathString} id={orderKey.idString}>
      <run_time>
        <at at={yyyymmddhhmmssFormatter.print(startTime)}/>
      </run_time>
    </add_order>
}
