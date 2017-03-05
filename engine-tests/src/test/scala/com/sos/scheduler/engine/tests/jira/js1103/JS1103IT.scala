package com.sos.scheduler.engine.tests.jira.js1103

import com.google.common.io.Files.touch
import com.sos.jobscheduler.common.scalautil.Closers.withCloser
import com.sos.jobscheduler.common.scalautil.FileUtils.implicits._
import com.sos.jobscheduler.common.scalautil.Futures.implicits._
import com.sos.jobscheduler.common.scalautil.SideEffect.ImplicitSideEffect
import com.sos.jobscheduler.common.time.ScalaTime._
import com.sos.jobscheduler.data.event.KeyedEvent
import com.sos.scheduler.engine.data.jobchain.JobChainPath
import com.sos.scheduler.engine.data.order.{OrderFinished, OrderId, OrderKey, OrderStarted}
import com.sos.scheduler.engine.data.xmlcommands.OrderCommand
import com.sos.scheduler.engine.kernel.async.SchedulerThreadCallQueue
import com.sos.scheduler.engine.kernel.async.SchedulerThreadFutures.inSchedulerThread
import com.sos.scheduler.engine.test.SchedulerTestUtils._
import com.sos.scheduler.engine.test.scalatest.ScalaSchedulerTest
import com.sos.scheduler.engine.tests.jira.js1103.JS1103IT._
import java.nio.file.Path
import org.junit.runner.RunWith
import org.scalatest.FreeSpec
import org.scalatest.junit.JUnitRunner

/**
  * @author Joacim Zschimmer
  */
@RunWith(classOf[JUnitRunner])
final class JS1103IT extends FreeSpec with ScalaSchedulerTest {

  private lazy val directory = testEnvironment.newFileOrderSourceDirectory()
  implicit private lazy val callQueue = controller.instance[SchedulerThreadCallQueue]

  "max_orders and ignore_max_orders" in {
    writeConfigurationFile(TestJobChainPath, newJobChain())
    val orderKeys = for (i ← 1 to TestOrderCount) yield TestJobChainPath orderKey s"$i"
    runOrders {
      for (o ← orderKeys) yield startOrder(OrderCommand(o))
    }
  }

  "max_orders, ignore_max_orders and file file_order_source" in {
    writeConfigurationFile(FileOrderJobChainPath, newJobChain(Some(directory)))
    val files = for (i ← 1 to TestOrderCount) yield directory / s"$i"
    runOrders {
      for (f ← files) yield OrderRun(FileOrderJobChainPath orderKey f.toString) sideEffect { _ ⇒ touch(f) }
    }
  }

  private def runOrders(callOrderRuns: ⇒ Iterable[OrderRun]): Unit =
    withCloser { implicit closer ⇒
      val totalCounter = new MaximumCounter
      val ordinaryCounter = new MaximumCounter
      eventBus.onHot[OrderStarted.type] {
        case KeyedEvent(OrderKey(_, OrderId(idString)), _) ⇒
          if (!(idString endsWith "-extra")) ordinaryCounter += 1
          totalCounter += 1
      }
      eventBus.onHot[OrderFinished] {
        case KeyedEvent(OrderKey(_, OrderId(idString)), _) ⇒
          if (!(idString endsWith "-extra")) ordinaryCounter -= 1
          totalCounter -= 1
      }
      val allOrderRuns = inSchedulerThread {  // Block C++ thread to let OrderRun timely subscribe to events
        val ordinaryOrderRuns = callOrderRuns
        val ordinaryOrderKeys = ordinaryOrderRuns map { _.orderKey }
        val extraOrderKeys = for (o ← ordinaryOrderKeys; i ← ExtraIndices) yield o.copy(id = OrderId(s"${o.id.string}-$i-extra"))
        ordinaryOrderRuns ++ (extraOrderKeys map OrderRun.apply)
      }
      allOrderRuns map { _.finished } await 60.s
      assert(ordinaryCounter.maximum == MaxOrders)
      assert(totalCounter.maximum > MaxOrders)
    }
}

private object JS1103IT {
  private val MaxOrders = 1
  private val TestOrderCount = 6
  private[js1103] val ExtraIndices = 1 to 3
  private val TestJobChainPath = JobChainPath("/test")
  private val FileOrderJobChainPath = JobChainPath("/fileOrder")

  private def newJobChain(directory: Option[Path] = None) =
    <job_chain max_orders={MaxOrders.toString}>
      { (for (d ← directory) yield <file_order_source directory={d.toString}/>).orNull }
      <job_chain_node state="100" job="test-100"/>
      <job_chain_node state="200" job="test-200"/>
      { (for (_ ← directory) yield <file_order_sink state="DELETE" remove="true"/>).orNull }
      <job_chain_node.end state="300-END"/>
      <job_chain_node state="400" job="test-100"/>
      <job_chain_node state="500" job="test-100"/>
      <job_chain_node.end state="END"/>
    </job_chain>
}
