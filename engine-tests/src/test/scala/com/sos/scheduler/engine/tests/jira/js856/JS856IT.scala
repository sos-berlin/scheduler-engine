package com.sos.scheduler.engine.tests.jira.js856

import com.sos.scheduler.engine.data.jobchain.JobChainPath
import com.sos.scheduler.engine.data.order._
import com.sos.scheduler.engine.kernel.order.OrderSubsystemClient
import com.sos.scheduler.engine.test.scalatest.ScalaSchedulerTest
import com.sos.scheduler.engine.tests.jira.js856.JS856IT._
import org.scalatest.FunSuite
import org.scalatest.Matchers._

/** JS-856 */
abstract class JS856IT(testNamePrefix: String) extends FunSuite with ScalaSchedulerTest {

  val finallyExpectedParameters: Map[String, String]
  val whenSuspendedExpectedParameters: Map[String, String]

  test(testNamePrefix +" - run standing order normally") {
    val c = new StandingOrderContext(testJobChain)
    c.runUntilEnd()
    c.orderParameters should equal (finallyExpectedParameters.toMap)
  }

  test(testNamePrefix +" - run standing order until suspended, then reset") {
    val c = new StandingOrderContext(suspendedJobChain)
    c.runUntilSuspendedThenReset()
    c.orderParameters should equal (finallyExpectedParameters)
  }

  test(testNamePrefix +" - run temporary order normally") {
    new TemporaryOrderContext(testJobChain).runUntilEnd()
    //Der Auftrag ist weg. Wie prüfen wir seine Parameter? c.orderParameters should equal (finallyExpectedParameters.toMap)
  }

  test(testNamePrefix +" - run temporary order until suspended, then reset") {
    val c = new TemporaryOrderContext(suspendedJobChain)
    c.runUntilSuspendedThenReset()
    c.orderParameters should equal (modifiedParameters)  // Nach Reset ist der Auftrag offenbar noch da ...
  }


  abstract class OrderContext(val orderKey: OrderKey) {
    final def runUntilEnd(): Unit = {
      val eventPipe = controller.newEventPipe()
      startOrder()
      eventPipe.nextAny[OrderFinishedEvent]
    }

    final def runUntilSuspendedThenReset(): Unit = {
      val eventPipe = controller.newEventPipe()
      startOrder(List(suspendedParameterName -> suspendedTrue))
      eventPipe.nextAny[OrderStepEndedEvent]
      eventPipe.nextAny[OrderSuspendedEvent]
      orderParameters should equal (whenSuspendedExpectedParameters)
      resetOrder()
    }

    def startOrder(parameters: Iterable[(String, String)] = Nil): Unit

    final def resetOrder(): Unit = {
      scheduler executeXml <modify_order job_chain={orderKey.jobChainPath.string} order={orderKey.id.string} action="reset"/>
    }

    final def orderParameters = order.variables filterKeys { _ != suspendedParameterName }

    final def order = instance[OrderSubsystemClient].order(orderKey)
  }

  class StandingOrderContext(jobChainPath: JobChainPath) extends OrderContext(jobChainPath orderKey "1") {
    final def startOrder(parameters: Iterable[(String, String)]): Unit = {
      orderParameters should equal (originalParameters)
      scheduler executeXml
          <modify_order job_chain={orderKey.jobChainPath.string} order={orderKey.id.string} at="now">{paramsElem(parameters)}</modify_order>
    }
  }

  class TemporaryOrderContext(jobChainPath: JobChainPath) extends OrderContext(jobChainPath orderKey "temporary") {
    final def startOrder(parameters: Iterable[(String, String)]): Unit = {
      scheduler executeXml <order job_chain={orderKey.jobChainPath.string} id={orderKey.id.string}>{paramsElem(parameters)}</order>
    }
  }
}

object JS856IT {
  val testJobChain = JobChainPath("/test")
  val suspendedJobChain = JobChainPath("/testSuspended")
  val originalParameters = Map("a" -> "a-original")
  val modifiedParameters = Map("a" -> "a-job", "b" -> "b-job")
  val suspendedParameterName = "suspended"
  val suspendedTrue = "true"

  private def paramsElem(parameters: Iterable[(String, String)]) =
    <params>{parameters map { case (n, v) => <param name={n} value={v}/> }}</params>
}
