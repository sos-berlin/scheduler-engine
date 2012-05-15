package com.sos.scheduler.engine.tests.jira.js856

import com.sos.scheduler.engine.data.order._
import com.sos.scheduler.engine.test.scala.ScalaSchedulerTest
import com.sos.scheduler.engine.test.scala.SchedulerTestImplicits._
import org.scalatest.matchers.ShouldMatchers._
import scala.collection.JavaConversions._
import com.sos.scheduler.engine.data.folder.JobChainPath

/** JS-856 */
abstract class JS856Test(testNamePrefix: String) extends ScalaSchedulerTest {
  import JS856Test._

  val finallyExpectedParameters: Map[String, String]
  val whenSuspendedExpectedParameters: Map[String, String]

  override def checkedBeforeAll() {
    controller.useDatabase()
  }

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
    final def runUntilEnd() {
      val eventPipe = controller.newEventPipe()
      startOrder()
      eventPipe.next[OrderFinishedEvent]
    }

    final def runUntilSuspendedThenReset() {
      val eventPipe = controller.newEventPipe()
      startOrder(List(suspendedParameterName -> suspendedTrue))
      eventPipe.next[OrderStepEndedEvent]
      eventPipe.next[OrderSuspendedEvent]
      orderParameters should equal (whenSuspendedExpectedParameters)
      resetOrder()
    }

    def startOrder(parameters: Iterable[(String, String)] = Nil)

    final def resetOrder() {
      scheduler executeXml <modify_order job_chain={orderKey.jobChainPathString} order={orderKey.idString} action="reset"/>
    }

    final def orderParameters = mapAsScalaMap(order.getParameters.toMap).toMap filterKeys { _ != suspendedParameterName }

    final def order = scheduler.getOrderSubsystem.order(orderKey)
  }

  class StandingOrderContext(jobChainPath: JobChainPath) extends OrderContext(new OrderKey(jobChainPath, new OrderId("1"))) {
    final def startOrder(parameters: Iterable[(String, String)]) {
      orderParameters should equal (originalParameters)
      scheduler executeXml
          <modify_order job_chain={orderKey.jobChainPathString} order={orderKey.idString} at="now">{paramsElem(parameters)}</modify_order>
    }
  }

  class TemporaryOrderContext(jobChainPath: JobChainPath) extends OrderContext(new OrderKey(jobChainPath, new OrderId("temporary"))) {
    final def startOrder(parameters: Iterable[(String, String)]) {
      scheduler executeXml <order job_chain={orderKey.jobChainPathString} id={orderKey.idString}>{paramsElem(parameters)}</order>
    }
  }
}

object JS856Test {
  val testJobChain = JobChainPath.of("/test")
  val suspendedJobChain = JobChainPath.of("/testSuspended")
  val originalParameters = Map("a" -> "a-original")
  val modifiedParameters = Map("a" -> "a-job", "b" -> "b-job")
  val suspendedParameterName = "suspended"
  val suspendedTrue = "true"

  private def paramsElem(parameters: Iterable[(String, String)]) =
    <params>{parameters map { case (n, v) => <param name={n} value={v}/> }}</params>
}
