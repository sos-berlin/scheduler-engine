package com.sos.scheduler.engine.tests.jira.js1642

import akka.util.Switch
import com.sos.scheduler.engine.base.convert.ConvertiblePartialFunctions.ImplicitConvertablePF
import com.sos.scheduler.engine.client.web.WebSchedulerClient
import com.sos.scheduler.engine.common.scalautil.Futures.implicits._
import com.sos.scheduler.engine.common.scalautil.Logger
import com.sos.scheduler.engine.common.time.Stopwatch
import com.sos.scheduler.engine.data.event.Snapshot
import com.sos.scheduler.engine.data.order.{OrderOverview, OrderStatistics}
import com.sos.scheduler.engine.data.queries.{JobChainQuery, OrderQuery}
import com.sos.scheduler.engine.data.xmlcommands.OrderCommand
import com.sos.scheduler.engine.kernel.DirectSchedulerClient
import com.sos.scheduler.engine.kernel.async.SchedulerThreadFutures.inSchedulerThread
import com.sos.scheduler.engine.kernel.order.OrderTester
import com.sos.scheduler.engine.test.scalatest.ScalaSchedulerTest
import com.sos.scheduler.engine.tests.jira.js1642.Data._
import com.sos.scheduler.engine.tests.jira.js1642.SpeedTests._
import org.scalatest.FreeSpec
import spray.http.MediaTypes._
import spray.http.{HttpData, HttpEntity}
import spray.httpx.unmarshalling.Unmarshaller

/**
  * @author Joacim Zschimmer
  */
private[js1642] trait SpeedTests {
  this: ScalaSchedulerTest with FreeSpec ⇒

  protected def directSchedulerClient: DirectSchedulerClient
  protected def webSchedulerClient: WebSchedulerClient

  def addOptionalSpeedTests() {
    for (n ← sys.props.optionAs[Int]("JS1642IT")) {
      "Speed test Order C++ accesses" - {
        "a1OrderKey" in {
          instance[OrderTester].testSpeed(a1OrderKey, 3, 10000)
        }
        "b1OrderKey with obstacle" in {
          instance[OrderTester].testSpeed(b1OrderKey, 3, 10000)  // With MissingRequisites
        }
        "OrderOverview" in {
          inSchedulerThread {
            var logged = new Switch
            for (_ ← 1 to 5) Stopwatch.measureTime(n, "OrderOverview") {
              val response = directSchedulerClient.ordersBy[OrderOverview](OrderQuery(JobChainQuery(isDistributed = Some(false)))).successValue
              logged switchOn { logger.info(response.toString) }
            }
          }
        }
      }

      s"Speed test with $n orders added" - {
        s"(Add $n orders)" in {
          val stopwatch = new Stopwatch
          for (orderKey ← 1 to n map { i ⇒ aJobChainPath orderKey s"adhoc-$i" })
            controller.scheduler executeXml OrderCommand(orderKey)
          logger.info("Adding orders: " + stopwatch.itemsPerSecondString(n, "order"))
        }

        "OrdersComplemented" in {
          for (_ ← 1 to 40) {
            val stopwatch = new Stopwatch
            implicit val fastUnmarshaller = Unmarshaller[HttpData](`application/json`) {
              case HttpEntity.NonEmpty(contentType, entity) ⇒ entity
            }
            webSchedulerClient.get[HttpData](_.order.complemented[OrderOverview](OrderQuery(JobChainQuery(isDistributed = Some(false))))) await TestTimeout
            val s = stopwatch.itemsPerSecondString(n, "order")
            logger.info(s"OrdersComplemented $testName: $s")
          }
        }

        "OrderStatistics" in {
          for (_ ← 1 to 5) {
            val stopwatch = new Stopwatch
            val Snapshot(_, orderStatistics: OrderStatistics) = webSchedulerClient.orderStatistics(JobChainQuery.All) await TestTimeout
            logger.info("OrderStatistics: " + stopwatch.itemsPerSecondString(n, "order"))
            assert(orderStatistics == OrderStatistics(
              total = n + 8,
              notPlanned = 0,
              planned = 1,
              due = n - 3,
              running = 10,
              inTask = 10,
              inProcess = 10,
              setback = 0,
              suspended = 2,
              blacklisted = 0,
              permanent = 6,
              fileOrder = 0))
          }
        }

        "<show_state>" in {
          for (_ ← 1 to 5) {
            val stopwatch = new Stopwatch
            webSchedulerClient.uncheckedExecute(<s/>) await TestTimeout
            val s = stopwatch.itemsPerSecondString(n, "order")
            logger.info(s"<s/> $testName: $s")
          }
        }
      }

      s"Speed test with $n distributed (database) orders added" - {
        s"(Add $n orders)" in {
          val stopwatch = new Stopwatch
          for (orderKey ← 1 to n map { i ⇒ xbJobChainPath orderKey s"distributed-adhoc-$i" })
            controller.scheduler executeXml OrderCommand(orderKey)
          logger.info("Adding orders: " + stopwatch.itemsPerSecondString(n, "order"))
        }

        "OrderStatistics" in {
          for (_ ← 1 to 5) {
            val stopwatch = new Stopwatch
            val Snapshot(_, orderStatistics: OrderStatistics) = webSchedulerClient.orderStatistics(JobChainQuery.All) await TestTimeout
            logger.info("OrderStatistics: " + stopwatch.itemsPerSecondString(n, "order"))
            assert(orderStatistics == OrderStatistics(
              total = 2*n + 8,
              notPlanned = 0,
              planned = 1,
              due = 2*n - 3,
              running = 10,
              inTask = 10,
              inProcess = 10,
              setback = 0,
              suspended = 2,
              blacklisted = 0,
              permanent = 6,
              fileOrder = 0))
          }
        }
      }
    }
  }
}

object SpeedTests {
  private val logger = Logger(getClass)
}
