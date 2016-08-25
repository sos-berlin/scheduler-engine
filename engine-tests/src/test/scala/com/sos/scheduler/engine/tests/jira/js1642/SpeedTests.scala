package com.sos.scheduler.engine.tests.jira.js1642

import com.sos.scheduler.engine.client.web.WebSchedulerClient
import com.sos.scheduler.engine.common.convert.ConvertiblePartialFunctions.ImplicitConvertablePF
import com.sos.scheduler.engine.common.scalautil.Futures.implicits._
import com.sos.scheduler.engine.common.scalautil.Logger
import com.sos.scheduler.engine.common.time.Stopwatch
import com.sos.scheduler.engine.data.order.OrderOverview
import com.sos.scheduler.engine.data.queries.OrderQuery
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
      "Speed test Order C++ accesses" in {
        instance[OrderTester].testSpeed(a1OrderKey, 3, 10000)
        inSchedulerThread {
          for (_ ← 1 to 5) Stopwatch.measureTime(100000, "OrderOverview") {
            directSchedulerClient.ordersBy[OrderOverview](OrderQuery(isDistributed = Some(false))).successValue
          }
        }
      }

      "Speed test: OrdersComplemented" - {
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
            webSchedulerClient.get[HttpData](_.order.complemented[OrderOverview](OrderQuery(isDistributed = Some(false)))) await TestTimeout
            val s = stopwatch.itemsPerSecondString(n, "order")
            logger.info(s"OrdersComplemented $testName: $s")
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
    }
  }
}

object SpeedTests {
  private val logger = Logger(getClass)
}
