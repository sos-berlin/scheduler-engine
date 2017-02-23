package com.sos.scheduler.engine.plugins.nodeorder

import com.sos.jobscheduler.common.scalautil.Closers._
import com.sos.jobscheduler.data.event.KeyedEvent
import com.sos.jobscheduler.data.message.MessageCode
import com.sos.scheduler.engine.data.jobchain.JobChainPath
import com.sos.scheduler.engine.data.log.ErrorLogged
import com.sos.scheduler.engine.data.order.OrderFinished
import com.sos.scheduler.engine.data.xmlcommands.OrderCommand
import com.sos.scheduler.engine.plugins.nodeorder.NodeOrderPlugin._
import com.sos.scheduler.engine.plugins.nodeorder.NodeOrderPluginIT._
import com.sos.scheduler.engine.test.EventBusTestFutures.implicits.RichEventBus
import com.sos.scheduler.engine.test.SchedulerTestUtils.{awaitSuccess, interceptSchedulerError, orderDetailed}
import com.sos.scheduler.engine.test.scalatest.ScalaSchedulerTest
import org.junit.runner.RunWith
import org.scalatest.FreeSpec
import org.scalatest.Matchers._
import org.scalatest.junit.JUnitRunner
import scala.concurrent.ExecutionContext.Implicits.global
import scala.concurrent.{Future, Promise}

/**
 * @see https://change.sos-berlin.com/browse/JS-1193
 * @author Joacim Zschimmer
 */
@RunWith(classOf[JUnitRunner])
final class NodeOrderPluginIT extends FreeSpec with ScalaSchedulerTest {

  "New orders are added" in {
    controller.toleratingErrorCodes(Set(MessageCode("SCHEDULER-280"))) {
      val promiseMap = (OrderKeys map { _ → Promise[Map[String, String]]() }).toMap
      withCloser { implicit closer ⇒
        eventBus.onHot[OrderFinished] {
          case KeyedEvent(orderKey, _) ⇒
            promiseMap(orderKey).success(orderDetailed(orderKey).variables)
        }
        scheduler executeXml OrderCommand(OriginalOrderKey, parameters = OriginalVariables)
        val results = awaitSuccess(Future.sequence(OrderKeys map promiseMap map { _.future }))
        assert((OrderKeys zip results) == ExpectedVariables)
      }
    }
  }

  "Error when adding the new order is logged and ignored" in {
    controller.toleratingErrorCodes(Set(MissingJobchainCode, CommandFailedCode)) {
      eventBus.awaiting[OrderFinished](ErrorOrderKey) {
        withEventPipe { eventPipe ⇒
          scheduler executeXml OrderCommand(ErrorOrderKey)
          eventPipe.nextWhen[ErrorLogged] { _.event.codeOption == Some(MissingJobchainCode) }
          eventPipe.nextWhen[ErrorLogged] { _.event.codeOption == Some(CommandFailedCode) }
        }
      }
    }
  }

  "<add_order> must not denote the own job-chain" in {
    interceptSchedulerError(MessageCode("Z-JAVA-105")) {
      scheduler executeXml
        <job_chain name="test-own"
                   xmlns:xsi="http://www.w3.org/2001/XMLSchema-instance"
                   xmlns:NodeOrderPlugin="https://jobscheduler-plugins.sos-berlin.com/NodeOrderPlugin">
          <job_chain_node state="100" job="/test-exit-0">
            <on_return_codes>
              <on_return_code return_code="0">
                <NodeOrderPlugin:add_order NodeOrderPlugin:job_chain="/test-own"/>
              </on_return_code>
            </on_return_codes>
          </job_chain_node>
          <job_chain_node.end state="end"/>
        </job_chain>
    } .getMessage should include ("must not denote the own job_chain")
  }
}

private object NodeOrderPluginIT {
  private val OriginalOrderKey = JobChainPath("/test-folder/a") orderKey "TEST"
  private val OriginalVariables = Map("a" → "A", "test" → "TEST")
  private val ExpectedVariables = List(
    OriginalOrderKey → OriginalVariables,
    (JobChainPath("/test-folder/b") orderKey "TEST") → OriginalVariables,   // Added by <NodeOrderPlugin:add_order NodeOrderPlugin:job_chain="/test-b"/>
    (JobChainPath("/test-folder-c/c") orderKey "TEST") → OriginalVariables, // Added by <NodeOrderPlugin:add_order NodeOrderPlugin:job_chain="/test-c"/>
    (JobChainPath("/test-folder/d") orderKey "aaaTESTzzz") → Map("test" → "TEST", "a" → "AAA", "b" → "BBB")  // Added by <NodeOrderPlugin:add_order NodeOrderPlugin:job_chain="/test-d" NodeOrderPlugin:id="aaa${ORDER_ID}zzz"/>
  )
  private val OrderKeys = ExpectedVariables map { _._1 }

  private val ErrorOrderKey = JobChainPath("/test-folder/error") orderKey "TEST"
  private val MissingJobchainCode = MessageCode("SCHEDULER-161")
}
