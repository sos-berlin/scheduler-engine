package com.sos.scheduler.engine.tests.agent.manytasks.a

import com.sos.scheduler.engine.common.scalautil.Futures.implicits._
import com.sos.scheduler.engine.common.scalautil.Logger
import com.sos.scheduler.engine.common.time.ScalaTime._
import com.sos.scheduler.engine.common.time.Stopwatch
import com.sos.scheduler.engine.data.jobchain.JobChainPath
import com.sos.scheduler.engine.data.order.OrderId
import com.sos.scheduler.engine.data.xmlcommands.OrderCommand
import com.sos.scheduler.engine.test.SchedulerTestUtils._
import com.sos.scheduler.engine.test.agent.AgentWithSchedulerTest
import com.sos.scheduler.engine.test.agent.AgentWithSchedulerTest.AgentProcessClassPath
import com.sos.scheduler.engine.test.scalatest.ScalaSchedulerTest
import com.sos.scheduler.engine.tests.agent.manytasks.a.ManyAgentTasksIT._
import org.junit.runner.RunWith
import org.scalatest.FreeSpec
import org.scalatest.junit.JUnitRunner

/**
  * @author Joacim Zschimmer
  */
@RunWith(classOf[JUnitRunner])
final class ManyAgentTasksIT extends FreeSpec with ScalaSchedulerTest with AgentWithSchedulerTest {

  private val n = sys.props.get("ManyAgentTasksIT").fold(100)(_.toInt)

  "Many quick tasks" in {
    testOrders(JobChainPath("/quick"))
  }

  "Many slow and parallel tasks" in {
    testOrders(JobChainPath("/one-minute"))
  }

  private def testOrders(jobChainPath: JobChainPath): Unit = {
    scheduler executeXml <process_class name={AgentProcessClassPath.withoutStartingSlash} remote_scheduler={agentUri} max_processes={n.toString}/>
    val orderIds = for (i ← 1 to n) yield OrderId(s"ORDER-$i")
    val stopwatch = new Stopwatch
    (for (orderId ← orderIds) yield
      startOrder(OrderCommand(jobChainPath orderKey orderId)).result
    ) await 600.s
    logger.info(jobChainPath.withoutStartingSlash + " " + stopwatch.itemsPerSecondString(n, "order"))
  }
}

private object ManyAgentTasksIT {
  private val logger = Logger(getClass)
}
