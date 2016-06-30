package com.sos.scheduler.engine.tests.jira.js1642

import com.sos.scheduler.engine.data.filebased.FileBasedState
import com.sos.scheduler.engine.data.jobchain.JobChainPath
import com.sos.scheduler.engine.data.order.{OrderOverview, OrderState}
import com.sos.scheduler.engine.data.xmlcommands.OrderCommand
import com.sos.scheduler.engine.test.scalatest.ScalaSchedulerTest
import com.sos.scheduler.engine.tests.jira.js1642.Provisioning._
import java.time.Instant
import java.time.Instant._

/**
  * @author Joacim Zschimmer
  */
private[js1642] trait Provisioning extends ScalaSchedulerTest {

  override protected def onSchedulerActivated() = {
    scheduler executeXml OrderCommand(
      JobChainPath("/aJobChain") orderKey "AD-HOC",
      at = Some(OrderStartAt),
      suspended = Some(true))
    super.onSchedulerActivated()
  }
}

private[js1642] object Provisioning {
  final val OrderStartAt = Instant.parse("2038-01-01T11:22:33Z")

  val ExpectedOrderViews = Vector(
    OrderOverview(
      JobChainPath("/aJobChain") orderKey "1",
      FileBasedState.active,
      OrderState("100"),
      nextStepAt = Some(EPOCH)),
    OrderOverview(
      JobChainPath("/aJobChain") orderKey "2",
      FileBasedState.active,
      OrderState("100"),
      nextStepAt = Some(EPOCH)),
    OrderOverview(
      JobChainPath("/aJobChain") orderKey "AD-HOC",
      FileBasedState.notInitialized,
      OrderState("100"),
      nextStepAt = Some(OrderStartAt),
      isSuspended = true),
    OrderOverview(
      JobChainPath("/bJobChain") orderKey "1",
      FileBasedState.active,
      OrderState("100"),
      nextStepAt = Some(EPOCH)),
    OrderOverview(
      JobChainPath("/aFolder/a-aJobChain") orderKey "1",
      FileBasedState.active,
      OrderState("100"),
      nextStepAt = Some(EPOCH)),
    OrderOverview(
      JobChainPath("/aFolder/a-aJobChain") orderKey "2",
      FileBasedState.active,
      OrderState("100"),
      nextStepAt = Some(EPOCH)),
    OrderOverview(
      JobChainPath("/aFolder/a-bJobChain") orderKey "1",
      FileBasedState.active,
      OrderState("100"),
      nextStepAt = Some(EPOCH)))

  val OrderCount = ExpectedOrderViews.size
}
