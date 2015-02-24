package com.sos.scheduler.engine.tests.jira.js803

import com.sos.scheduler.engine.common.scalautil.Logger
import com.sos.scheduler.engine.data.jobchain.JobChainPath
import com.sos.scheduler.engine.data.order._
import com.sos.scheduler.engine.eventbus.{EventHandler, HotEventHandler}
import com.sos.scheduler.engine.kernel.order._
import com.sos.scheduler.engine.test.SchedulerTest
import com.sos.scheduler.engine.tests.jira.js803.JS803IT._
import org.hamcrest.MatcherAssert.assertThat
import org.hamcrest.Matchers.equalTo
import org.joda.time.DateTime
import org.joda.time.format.DateTimeFormat
import org.junit.Assert._
import org.junit.Test
import scala.collection.mutable
import scala.xml.Utility.trim

/** Ticket JS-803.
 * @see <a href='http://www.sos-berlin.com/jira/browse/JS-803'>JS-803</a>
 * @see com.sos.scheduler.engine.tests.jira.js653.JS653IT */
final class JS803IT extends SchedulerTest {

  private val expectedOrders = new mutable.HashSet[OrderId]
  private val terminatedOrders = new mutable.HashSet[OrderId]
  private lazy val startTime = secondNow() plusSeconds orderDelay

  @Test def test(): Unit = {
    controller.activateScheduler()
    addOrder(jobChainPath orderKey "dailyOrder", addDailyOrderElem)
    addOrder(jobChainPath orderKey "singleOrder", addSingleOrderElem)
    addOrder(jobChainPath orderKey "singleRuntimeOrder", addSingleRuntimeOrderElem)
    try controller.waitForTermination()
    finally (expectedOrders diff terminatedOrders).toList match {
      case List() =>
      case notTerminatedOrders => logger.error(s"Orders failed to terminate: ${notTerminatedOrders mkString ", "}")
    }
  }

  private def addOrder(orderKey: OrderKey, orderElemFunction: (OrderKey, DateTime) => xml.Elem): Unit = {
    execute(orderElemFunction(orderKey, startTime))
    expectedOrders.add(orderKey.id)
  }

  private def execute(command: xml.Elem): Unit = {
    logger.debug(trim(command).toString())
    scheduler.executeXml(command)
  }

  @EventHandler def handleEvent(e: OrderTouchedEvent): Unit = {
    assertTrue(s"Order ${e.orderKey} has been started before expected time $startTime", !(new DateTime isBefore startTime))
  }

  @HotEventHandler def handleHotEvent(event: OrderFinishedEvent, order: UnmodifiableOrder): Unit =
    assertThat(s"Wrong end state of order ${event.orderKey}", order.state, equalTo(expectedEndState))

  @EventHandler def handleEvent(event: OrderFinishedEvent): Unit = {
    terminatedOrders.add(event.orderKey.id)
    if (terminatedOrders == expectedOrders)  controller.terminateScheduler()
  }
}

object JS803IT {
  private val logger = Logger(getClass)
  private val orderDelay = 3+1
  private val jobChainPath = JobChainPath("/super")
  private val expectedEndState = OrderState("state.nestedC.end")
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
