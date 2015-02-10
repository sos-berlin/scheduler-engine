package com.sos.scheduler.engine.kernel.order.jobchain

import com.sos.scheduler.engine.common.scalautil.AutoClosing.autoClosing
import com.sos.scheduler.engine.common.scalautil.ScalaUtils.implicits.ToStringFunction1
import com.sos.scheduler.engine.common.scalautil.xmls.{XmlElemSource, ScalaXMLEventReader}
import com.sos.scheduler.engine.data.order.{ErrorOrderStateTransition, OrderState, SuccessOrderStateTransition}
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
    x.initializeWithNodeXml(XmlElemSource(JobchainNodeElem), Map(TestNamespace → testNamespaceParse _).lift)
    x.orderStateTransitionToState(SuccessOrderStateTransition) shouldEqual State0
    x.orderStateTransitionToState(ErrorOrderStateTransition(1)) shouldEqual State1
    x.orderStateTransitionToState(ErrorOrderStateTransition(7)) shouldEqual State7
    x.orderStateTransitionToState(ErrorOrderStateTransition(99)) shouldEqual ErrorState
    x.returnCodeToOrderFunctions(0) shouldEqual Nil
    x.returnCodeToOrderFunctions(1) shouldEqual Nil
    x.returnCodeToOrderFunctions(2) shouldEqual Nil
    x.returnCodeToOrderFunctions(7) should have size 1
    x.returnCodeToOrderFunctions(7).head.toString shouldEqual TestCallbackName
  }
}

private object JobChainNodeParserAndHandlerTest {
  private val CurrentState = OrderState("CURRENT")
  private val NextState = OrderState("NEXT")
  private val ErrorState = OrderState("ERROR")
  private val State0 = OrderState("STATE-0")
  private val State1 = OrderState("STATE-1")
  private val State7 = OrderState("STATE-7")
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
          <to_state state={State0.string}/>
        </on_return_code>
        <on_return_code return_code="1">
          <Ignored:ignored/>
          <to_state state={State1.string}/>
        </on_return_code>
        <on_return_code return_code="7">
          <Test:TEST/>
          <to_state state={State7.string}/>
        </on_return_code>
      </on_return_codes>
    </job_chain_node>

  private class X extends JobChainNodeParserAndHandler {
    protected def orderState = CurrentState
    protected def nextState = NextState
    protected def errorState = ErrorState
  }

  private def testNamespaceParse(xmlEventReader: XMLEventReader): OrderFunction =
    autoClosing(new ScalaXMLEventReader(xmlEventReader)) { r ⇒
    r.parseElement("TEST") {}
    ;{ _: Order ⇒ } withToString TestCallbackName
  }
}
