package com.sos.scheduler.engine.tests.jira.js1026

import com.sos.jobscheduler.data.event.KeyedEvent
import com.sos.scheduler.engine.data.order.OrderFinished
import com.sos.scheduler.engine.test.SchedulerTestUtils.orderDetailed
import com.sos.scheduler.engine.test.scalatest.ScalaSchedulerTest
import com.sos.scheduler.engine.test.util.CommandBuilder
import org.junit.runner.RunWith
import org.scalatest.FreeSpec
import org.scalatest.junit.JUnitRunner

/**
  * This is a test for SCHEDULER_RETURN_VALUES in order shell jobs.
  * The test uses jobchain chain.
  * The order jobs test1_order and test2_order will be started.
  *
  * @author Florian Schreiber
  * @version 1.0 - 24.09.2013 13:39:41
  */
@RunWith(classOf[JUnitRunner])
final class JS1026ShellOrderIT extends FreeSpec with ScalaSchedulerTest {
  private val util = new CommandBuilder

  "test" in {
    controller.scheduler.executeXml(util.addOrder(JS1026ShellOrderIT.jobchain).getCommand)
  }

  eventBus.onHot[OrderFinished] {
    case KeyedEvent(orderKey, _) ⇒
      val variables = orderDetailed(orderKey).variables
      assert(variables("testvar1") == "value1")
      assert(variables("testvar2") == "newvalue2")
      assert(variables("testvar3") == "value3")
      assert(variables("testvar4") == "newvalue4")
      assert(variables("testvar5") == "value5")
      controller.terminateScheduler()
    }
}

private object JS1026ShellOrderIT {
  private val jobchain = "chain"
}
