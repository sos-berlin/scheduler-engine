package com.sos.scheduler.engine.tests.jira.js1693

import com.sos.scheduler.engine.common.scalautil.Futures.implicits._
import com.sos.scheduler.engine.data.event.KeyedEvent
import com.sos.scheduler.engine.data.jobchain.JobChainPath
import com.sos.scheduler.engine.data.order.OrderFinished
import com.sos.scheduler.engine.data.xmlcommands.OrderCommand
import com.sos.scheduler.engine.test.SchedulerTestUtils._
import com.sos.scheduler.engine.test.scalatest.ScalaSchedulerTest
import com.sos.scheduler.engine.tests.jira.js1693.JS1693IT._
import org.junit.runner.RunWith
import org.scalatest.FreeSpec
import org.scalatest.junit.JUnitRunner
import scala.concurrent.Promise

/**
  * @author Joacim Zschimmer
  */
@RunWith(classOf[JUnitRunner])
final class JS1693IT extends FreeSpec with ScalaSchedulerTest {

  private val permanentOrderFinished = Promise[Map[String, String]]()

  override protected def onBeforeSchedulerActivation() = {
    eventBus.onHot[OrderFinished] {
      case KeyedEvent(PermanentOrderKey, _: OrderFinished) ⇒
        permanentOrderFinished.success(orderDetailed(PermanentOrderKey).variables)
    }
    super.onBeforeSchedulerActivation()
  }

  "Environment variables in permanent order" in {
    val variables = permanentOrderFinished.future await TestTimeout
    orderLog(PermanentOrderKey)
    val expected = sys.env.getOrElse("PATH", sys.env("Path"))
    assert(variables("testpath") == s"/$expected/")
    assert(variables("included") == s"|$expected|")
  }

  "Environment variables in ad-hoc order" in {
    val variables = runOrder(OrderCommand(JobChainPath("/test") orderKey "Ad-hoc", parameters = Map("testpath" → "-$PATH-"))).variables
    val expected = sys.env.getOrElse("PATH", sys.env("Path"))
    assert(variables("testpath") == s"-$expected-")
  }
}

private object JS1693IT {
  private val PermanentOrderKey = JobChainPath("/test") orderKey "1"
}
