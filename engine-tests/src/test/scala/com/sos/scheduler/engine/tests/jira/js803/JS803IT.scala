package com.sos.scheduler.engine.tests.jira.js803

import com.sos.jobscheduler.common.scalautil.Logger
import com.sos.jobscheduler.data.event.KeyedEvent
import com.sos.scheduler.engine.data.jobchain.{JobChainPath, NodeId}
import com.sos.scheduler.engine.data.order._
import com.sos.scheduler.engine.test.scalatest.ScalaSchedulerTest
import com.sos.scheduler.engine.tests.jira.js803.JS803IT._
import org.joda.time.DateTime
import org.joda.time.format.DateTimeFormat
import org.junit.runner.RunWith
import org.scalatest.FreeSpec
import org.scalatest.junit.JUnitRunner
import scala.collection.mutable
import scala.xml.Utility.trim

/** Ticket JS-803.
 * @see <a href='http://www.sos-berlin.com/jira/browse/JS-803'>JS-803</a>
 * @see com.sos.scheduler.engine.tests.jira.js653.JS653IT */
@RunWith(classOf[JUnitRunner])
final class JS803IT extends FreeSpec with ScalaSchedulerTest {

  private val expectedOrders = new mutable.HashSet[OrderId]
  private val terminatedOrders = new mutable.HashSet[OrderId]
  private lazy val startTime = secondNow() plusSeconds orderDelay

  "test" in {
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

  eventBus.on[OrderStarted.type] {
    case KeyedEvent(orderKey, _) ⇒
      withClue(s"Order $orderKey has been started before expected time $startTime: ") {
        assert(!(new DateTime isBefore startTime))
      }
  }

  eventBus.on[OrderFinished] {
    case KeyedEvent(orderKey, event) ⇒
      assert(event.nodeId == expectedEndNodeId)
  }

  eventBus.on[OrderFinished] {
    case KeyedEvent(orderKey, _) ⇒
      terminatedOrders.add(orderKey.id)
      if (terminatedOrders == expectedOrders)  controller.terminateScheduler()
  }
}

object JS803IT {
  private val logger = Logger(getClass)
  private val orderDelay = 3+1
  private val jobChainPath = JobChainPath("/super")
  private val expectedEndNodeId = NodeId("state.nestedC.end")
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
