package com.sos.scheduler.engine.tests.jira.js856

import com.sos.scheduler.engine.data.folder.AbsolutePath
import com.sos.scheduler.engine.data.order.{OrderId, OrderFinishedEvent}
import com.sos.scheduler.engine.test.scala.ScalaSchedulerTest
import com.sos.scheduler.engine.test.scala.SchedulerTestImplicits._
import org.scalatest.matchers.ShouldMatchers._
import scala.collection.JavaConversions._

/** JS-856 */
abstract class JS856Test extends ScalaSchedulerTest {
  protected def expectFinishedParameters(expected: Iterable[(String,String)]) {
    val eventPipe = controller.newEventPipe()
    orderParameters() should equal (Map("a" -> "a-original"))
    scheduler executeXml <modify_order job_chain="/a" order="1" at="now"/>
    eventPipe.next[OrderFinishedEvent]
    orderParameters() should equal (expected.toMap)
  }

  private def orderParameters() = mapAsScalaMap(order().getParameters.toMap).toMap

  private def order() = scheduler.getOrderSubsystem.jobChain(new AbsolutePath("/a")).order(new OrderId("1"))
}
