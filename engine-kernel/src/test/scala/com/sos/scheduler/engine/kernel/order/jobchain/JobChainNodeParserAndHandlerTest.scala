package com.sos.scheduler.engine.kernel.order.jobchain

import com.sos.scheduler.engine.base.utils.ScalaUtils.implicits.ToStringFunction1
import com.sos.scheduler.engine.common.scalautil.AutoClosing.autoClosing
import com.sos.scheduler.engine.common.scalautil.xmls.ScalaXMLEventReader
import com.sos.scheduler.engine.common.scalautil.xmls.XmlSources.xmlElemToSource
import com.sos.scheduler.engine.data.job.ReturnCode
import com.sos.scheduler.engine.data.jobchain.NodeId
import com.sos.scheduler.engine.data.order.OrderNodeTransition
import com.sos.scheduler.engine.kernel.order.Order
import com.sos.scheduler.engine.kernel.order.jobchain.JobChainNodeParserAndHandler.OrderFunction
import com.sos.scheduler.engine.kernel.order.jobchain.JobChainNodeParserAndHandlerTest._
import javax.xml.stream.XMLEventReader
import org.junit.runner.RunWith
import org.scalatest.FreeSpec
import org.scalatest.Matchers._
import org.scalatest.junit.JUnitRunner

/**
 * @author Joacim Zschimmer
 */
@RunWith(classOf[JUnitRunner])
final class JobChainNodeParserAndHandlerTest extends FreeSpec {

  "JobChainNodeParserAndHandler" in {
    System.err.println(JobchainNodeElem.toString)
    val x = new X
    x.initializeWithNodeXml(JobchainNodeElem, Map(TestNamespace → testNamespaceParse _).lift)
    x.orderStateTransitionToState(OrderNodeTransition.Success) shouldEqual Some(NodeId0)
    x.orderStateTransitionToState(OrderNodeTransition.Error(ReturnCode(1))) shouldEqual Some(NodeId1)
    x.orderStateTransitionToState(OrderNodeTransition.Error(ReturnCode(7))) shouldEqual Some(NodeId7)
    x.orderStateTransitionToState(OrderNodeTransition.Error(ReturnCode(99))) shouldEqual None
    x.returnCodeToOrderFunctions(ReturnCode(0)) shouldEqual Nil
    x.returnCodeToOrderFunctions(ReturnCode(1)) shouldEqual Nil
    x.returnCodeToOrderFunctions(ReturnCode(2)) shouldEqual Nil
    x.returnCodeToOrderFunctions(ReturnCode(7)) should have size 1
    x.returnCodeToOrderFunctions(ReturnCode(7)).head.toString shouldEqual TestCallbackName
  }
}

private object JobChainNodeParserAndHandlerTest {
  private val CurrentNodeId = NodeId("CURRENT")
  private val NextNodeId = NodeId("NEXT")
  private val ErrorNodeId = NodeId("ERROR")
  private val NodeId0 = NodeId("STATE-0")
  private val NodeId1 = NodeId("STATE-1")
  private val NodeId7 = NodeId("STATE-7")
  private val TestNamespace = "https://EXAMPLE.COM/TEST"
  private val IgnoredNamespace = "http://EXAMPLE.COM/UNKNOWN"
  private val TestCallbackName = "TEST-CALLBACK"

  private val JobchainNodeElem =
    <job_chain_node
        xmlns:xsi="http://www.w3.org/2001/XMLSchema-instance"
        xmlns:Test={TestNamespace}
        xmlns:Ignored={IgnoredNamespace}>
      <on_return_codes>
        <on_return_code return_code="0">
          <to_state state={NodeId0.string}/>
        </on_return_code>
        <on_return_code return_code="1">
          <Ignored:ignored/>
          <to_state state={NodeId1.string}/>
        </on_return_code>
        <on_return_code return_code="7">
          <Test:TEST/>
          <to_state state={NodeId7.string}/>
        </on_return_code>
      </on_return_codes>
    </job_chain_node>

  private class X extends JobChainNodeParserAndHandler {
    def nodeId = CurrentNodeId
    def nextNodeId = NextNodeId
    def errorNodeId = ErrorNodeId
  }

  private def testNamespaceParse(xmlEventReader: XMLEventReader): OrderFunction =
    autoClosing(new ScalaXMLEventReader(xmlEventReader)) { r ⇒
    r.parseElement("TEST") {}
    ;{ _: Order ⇒ } withToString TestCallbackName
  }
}
