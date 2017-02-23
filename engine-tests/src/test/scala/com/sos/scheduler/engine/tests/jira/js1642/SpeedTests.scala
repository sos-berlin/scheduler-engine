package com.sos.scheduler.engine.tests.jira.js1642

import akka.util.Switch
import com.sos.jobscheduler.base.convert.ConvertiblePartialFunctions.ImplicitConvertablePF
import com.sos.scheduler.engine.client.web.WebSchedulerClient
import com.sos.jobscheduler.common.scalautil.Futures.implicits._
import com.sos.jobscheduler.common.scalautil.Logger
import com.sos.jobscheduler.common.time.Stopwatch
import com.sos.jobscheduler.data.event.Snapshot
import com.sos.scheduler.engine.data.order.{JocOrderStatistics, OrderDetailed, OrderOverview}
import com.sos.scheduler.engine.data.queries.{JobChainNodeQuery, JobChainQuery, OrderQuery}
import com.sos.scheduler.engine.data.xmlcommands.OrderCommand
import com.sos.scheduler.engine.kernel.DirectSchedulerClient
import com.sos.scheduler.engine.kernel.async.SchedulerThreadFutures.inSchedulerThread
import com.sos.scheduler.engine.kernel.order.OrderTester
import com.sos.scheduler.engine.test.scalatest.ScalaSchedulerTest
import com.sos.scheduler.engine.tests.jira.js1642.Data._
import com.sos.scheduler.engine.tests.jira.js1642.SpeedTests._
import org.scalatest.FreeSpec
import scala.concurrent.ExecutionContext.Implicits.global
import spray.http.MediaTypes._
import spray.http.{HttpData, HttpEntity, Uri}
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
      val nonDistributedQuery = OrderQuery(JobChainNodeQuery(JobChainQuery(isDistributed = Some(false))))  // 6 orders ?

      "orders[OrderOverview] speed" in {
        for (_ ← 1 to 20) {
          val stopwatch = new Stopwatch
          val n = 1000
          (for (_ ← 1 to n) yield webSchedulerClient.get[String](_.order[OrderOverview](nonDistributedQuery))) await TestTimeout
          logger.info(stopwatch.itemsPerSecondString(n, "OrderOverview"))
        }
      }

      "ordersComplemented speed" in {
        for (_ ← 1 to 20) {
          val stopwatch = new Stopwatch
          val n = 1000
          (for (_ ← 1 to n) yield webSchedulerClient.get[String](_.order.complemented[OrderOverview](nonDistributedQuery))) await TestTimeout
          logger.info(stopwatch.itemsPerSecondString(n, "OrdersComplemented[OrderOverview]"))
        }
      }

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

        "OrdersComplemented/OrdersOverview" in {
          for (_ ← 1 to 40) {
            val stopwatch = new Stopwatch
            implicit val fastUnmarshaller = Unmarshaller[HttpData](`application/json`) {
              case HttpEntity.NonEmpty(contentType, entity) ⇒ entity
            }
            webSchedulerClient.get[HttpData](_.order.complemented[OrderOverview](OrderQuery(JobChainQuery(isDistributed = Some(false))))) await TestTimeout
            val s = stopwatch.itemsPerSecondString(n, "order")
            logger.info(s"OrdersComplemented/OrderOverview $testName: $s")
          }
        }

        "OrdersComplemented/OrdersDetailed" in {
          for (_ ← 1 to 40) {
            val stopwatch = new Stopwatch
            implicit val fastUnmarshaller = Unmarshaller[HttpData](`application/json`) {
              case HttpEntity.NonEmpty(contentType, entity) ⇒ entity
            }
            webSchedulerClient.get[HttpData](_.order.complemented[OrderDetailed](OrderQuery(JobChainQuery(isDistributed = Some(false))))) await TestTimeout
            val s = stopwatch.itemsPerSecondString(n, "order")
            logger.info(s"OrdersComplemented/OrderDetailed $testName: $s")
          }
        }

        "OrderHtmlPage" in {
          for (_ ← 1 to 40) {
            val stopwatch = new Stopwatch
            implicit val fastUnmarshaller = Unmarshaller[HttpData](`text/html`) {
              case HttpEntity.NonEmpty(contentType, entity) ⇒ entity
            }
            webSchedulerClient.get[HttpData](_.uriString(Uri.Path("api/order/"), "isDistributed" → "false"), accept = `text/html`) await TestTimeout
            val s = stopwatch.itemsPerSecondString(n, "order")
            logger.info(s"OrderHtmlPage $testName: $s")
          }
        }

        "JocOrderStatistics" in {
          for (_ ← 1 to 5) {
            val stopwatch = new Stopwatch
            val Snapshot(_, orderStatistics: JocOrderStatistics) = webSchedulerClient.jocOrderStatistics(JobChainQuery.All) await TestTimeout
            logger.info("JocOrderStatistics: " + stopwatch.itemsPerSecondString(n, "order"))
            assert(orderStatistics == JocOrderStatistics(
              total = n + 8,
              notPlanned = 0,
              planned = 0,
              due = n - 4,
              started = 10,
              inTask = 10,
              inTaskProcess = 10,
              occupiedByClusterMember = 0,
              setback = 0,
              waitingForResource = 0,
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

        "JocOrderStatistics" in {
          for (_ ← 1 to 20) {
            val stopwatch = new Stopwatch
            val Snapshot(_, orderStatistics: JocOrderStatistics) = webSchedulerClient.jocOrderStatistics(JobChainQuery.All) await TestTimeout
            logger.info("JocOrderStatistics: " + stopwatch.itemsPerSecondString(n, "order"))
            assert(orderStatistics == JocOrderStatistics(
              total = 2*n + 8,
              notPlanned = 0,
              planned = 0,
              due = 2*n - 4,
              started = 10,
              inTask = 10,
              inTaskProcess = 10,
              occupiedByClusterMember = 0,
              setback = 0,
              waitingForResource = 0,
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
