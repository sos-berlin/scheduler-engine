package com.sos.scheduler.engine.tests.jira.js803

import com.sos.scheduler.engine.data.folder.JobChainPath
import com.sos.scheduler.engine.data.order._
import com.sos.scheduler.engine.eventbus.{HotEventHandler, EventHandler}
import com.sos.scheduler.engine.kernel.order._
import com.sos.scheduler.engine.test.SchedulerTest
import com.sos.scheduler.engine.test.scala.SchedulerTestImplicits._
import org.hamcrest.MatcherAssert.assertThat
import org.hamcrest.Matchers.equalTo
import org.joda.time.DateTime
import org.joda.time.format.DateTimeFormat
import org.junit.Assert._
import org.junit.Test
import org.slf4j.LoggerFactory
import scala.collection.mutable
import scala.xml.Elem
import scala.xml.Utility.trim

/** Ticket JS-803.
 * @see <a href='http://www.sos-berlin.com/jira/browse/JS-803'>JS-803</a>
 * @see com.sos.scheduler.engine.tests.jira.js653.JS653IT */
final class JS803IT extends SchedulerTest {

  import JS803IT._

  private val expectedOrders = new mutable.HashSet[OrderId]
  private val terminatedOrders = new mutable.HashSet[OrderId]
  private var startTime: DateTime = null

  @Test def test() {
    controller.activateScheduler()
    startTime = secondNow() plusSeconds orderDelay
    addOrder(OrderKey(jobChainPath, OrderId("dailyOrder")), addDailyOrderElem)
    addOrder(OrderKey(jobChainPath, OrderId("singleOrder")), addSingleOrderElem)
    addOrder(OrderKey(jobChainPath, OrderId("singleRuntimeOrder")), addSingleRuntimeOrderElem)
    try controller.waitForTermination(shortTimeout)
    finally (expectedOrders diff terminatedOrders).toList match {
      case List() =>
      case notTerminatedOrders => logger.error("Orders failed to terminate: "+ (notTerminatedOrders mkString ", "))
    }
  }

  private def addOrder(orderKey: OrderKey, orderElemFunction: (OrderKey, DateTime) => Elem) {
    execute(orderElemFunction(orderKey, startTime))
    expectedOrders.add(orderKey.id)
  }

  private def execute(command: Elem) {
    logger.debug("{}", trim(command))
    controller.scheduler.executeXml(command)
  }

  @EventHandler def handleEvent(e: OrderTouchedEvent) {
    assertTrue("Order "+e.orderKey+ " has been started before expected time "+startTime, new DateTime() isAfter startTime)
  }

  @HotEventHandler def handleHotEvent(event: OrderFinishedEvent, order: UnmodifiableOrder) {
    assertThat("Wrong end state of order "+event.orderKey, order.state, equalTo(expectedEndState))
  }

  @EventHandler def handleEvent(event: OrderFinishedEvent) {
    terminatedOrders.add(event.orderKey.id)
    if (terminatedOrders == expectedOrders)  controller.terminateScheduler()
  }
}

object JS803IT {
  private val logger = LoggerFactory.getLogger(classOf[JS803IT])
  private val shortTimeout = SchedulerTest.shortTimeout
  private val orderDelay = 3+1
  private val jobChainPath = JobChainPath("/super")
  private val expectedEndState = new OrderState("state.nestedC.end")
  private val hhmmssFormatter = DateTimeFormat.forPattern("HH:mm:ss")
  private val yyyymmddhhmmssFormatter = DateTimeFormat.forPattern("yyyy-MM-dd HH:mm:ss")

  private def secondNow() = {
    val now = new DateTime()
    now minusMillis now.millisOfSecond.get
  }

  private def addDailyOrderElem(orderKey: OrderKey, startTime: DateTime) =
    <add_order job_chain={orderKey.jobChainPath.string} id={orderKey.id.string}>
      <run_time>
        <period single_start={hhmmssFormatter.print(startTime)}/>
      </run_time>
    </add_order>

  private def addSingleOrderElem(orderKey: OrderKey, startTime: DateTime) =
    <add_order job_chain={orderKey.jobChainPath.string} id={orderKey.id.string}
               at={yyyymmddhhmmssFormatter.print(startTime)}/>

  private def addSingleRuntimeOrderElem(orderKey: OrderKey, startTime: DateTime) =
    <add_order job_chain={orderKey.jobChainPath.string} id={orderKey.id.string}>
      <run_time>
        <at at={yyyymmddhhmmssFormatter.print(startTime)}/>
      </run_time>
    </add_order>
}
