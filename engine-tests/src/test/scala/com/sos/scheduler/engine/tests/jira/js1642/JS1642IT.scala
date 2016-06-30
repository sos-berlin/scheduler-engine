package com.sos.scheduler.engine.tests.jira.js1642

import com.sos.scheduler.engine.common.scalautil.Futures.implicits._
import com.sos.scheduler.engine.common.time.Stopwatch
import com.sos.scheduler.engine.data.scheduler.{SchedulerId, SchedulerState}
import com.sos.scheduler.engine.kernel.order.OrderSubsystem
import com.sos.scheduler.engine.test.json.JsonRegexMatcher._
import com.sos.scheduler.engine.test.scalatest.ScalaSchedulerTest
import com.sos.scheduler.engine.tests.jira.js1642.Provisioning._
import org.junit.runner.RunWith
import org.scalatest.FreeSpec
import org.scalatest.junit.JUnitRunner
import spray.json.pimpAny

/**
  * @author Joacim Zschimmer
  */
@RunWith(classOf[JUnitRunner])
final class JS1642IT extends FreeSpec with ScalaSchedulerTest with Provisioning {
  private lazy val orderSubsystem = instance[OrderSubsystem]

  "Scheduler.overview" in {
    val overview = scheduler.overviewFuture await TestTimeout
    checkRegexJson(
      json = overview.toJson.toString,
      patternMap = Map(
        "version" → """\d+\..+""".r,
        "versionCommitHash" → ".*".r,
        "startInstant" → AnyIsoTimestamp,
        "instant" → AnyIsoTimestamp,
        "schedulerId" → "test",
        "pid" → AnyInt,
        "state" → "running"))
    assert(overview.schedulerId == SchedulerId("test"))
    assert(overview.state == SchedulerState.running)
  }

  "OrderSubsystem.orderOverviews" in {
    val orders = orderSubsystem.orderOverviews await TestTimeout
    assert(orders.toVector.sortBy { _.path } == ExpectedOrderViews.sortBy { _.path })
  }

  "OrderSubsystem.orderOverviews speed" in {
    Stopwatch.measureTime(1000, s""""orderOverviews with $OrderCount orders"""") {
      orderSubsystem.orderOverviews await TestTimeout
    }
  }
}
