package com.sos.scheduler.engine.tests.jira.js1040

import JS1040IT._
import com.sos.scheduler.engine.common.scalautil.AutoClosing.autoClosing
import com.sos.scheduler.engine.data.folder.{FileBasedRemovedEvent, FileBasedActivatedEvent, JobChainPath}
import com.sos.scheduler.engine.data.xmlcommands.OrderCommand
import com.sos.scheduler.engine.kernel.order.OrderSubsystem
import com.sos.scheduler.engine.test.scala.ScalaSchedulerTest
import com.sos.scheduler.engine.test.scala.SchedulerTestImplicits._
import org.junit.runner.RunWith
import org.scalatest.FreeSpec
import org.scalatest.junit.JUnitRunner

@RunWith(classOf[JUnitRunner])
final class JS1040IT extends FreeSpec with ScalaSchedulerTest {

  private lazy val orderSubsystem = instance[OrderSubsystem]

  "job_chain orders_recoverable=no should delete all orders in database when job chain is deleted and reread" - {
    addTests(removeBeforeReread = true)
  }

  "job_chain orders_recoverable=no should delete all orders in database when job chain is reread" - {
    addTests(removeBeforeReread = false)
  }

  private def addTests(removeBeforeReread: Boolean) {
    "Precondition: Job chain with orders_recoverable=yes should contain orders" in {
      readJobChain(ordersAreRecoverable = true)
      for (o <- orderKeys) scheduler executeXml OrderCommand(o)
      assertOrdersExists(shouldBeEmpty = false)
    }

    "Rereading Job chain with orders_recoverable=yes should keep orders" in {
      readJobChain(ordersAreRecoverable = true)
      assertOrdersExists(shouldBeEmpty = false)
    }

    "Rereading Job chain with orders_recoverable=no should not have orders" in {
      readJobChain(ordersAreRecoverable = false)
      assertOrdersExists(shouldBeEmpty = removeBeforeReread)
    }

    "Rereading Job chain with orders_recoverable=yes should too not have orders (new behavior with JS-1040)" in {
      readJobChain(ordersAreRecoverable = true)
      assertOrdersExists(shouldBeEmpty = removeBeforeReread)
    }

    def readJobChain(ordersAreRecoverable: Boolean) {
      if (removeBeforeReread && orderSubsystem.jobChainOption(testJobChainPath).isDefined)
        removeJobChain()
      autoClosing(controller.newEventPipe()) { eventPipe =>
        scheduler executeXml jobChainElem(ordersAreRecoverable)
        eventPipe.nextKeyed[FileBasedActivatedEvent](testJobChainPath)
      }
    }

    def removeJobChain() {
      autoClosing(controller.newEventPipe()) { eventPipe =>
        orderSubsystem.removeJobChain(testJobChainPath)
        controller.getEventBus.dispatchEvents()
        eventPipe.nextKeyed[FileBasedRemovedEvent](testJobChainPath)
      }
    }
  }

  private def assertOrdersExists(shouldBeEmpty: Boolean) {
    for (o <- orderKeys) {
      val hasNotOrder = orderSubsystem.orderOption(o).isEmpty
      if (hasNotOrder != shouldBeEmpty)
        fail(s"Job chain does ${if (hasNotOrder) "not " else ""}contain order $o")
    }
  }
}

private object JS1040IT {
  private val testJobChainPath = JobChainPath("/test")
  private val orderKeys = List("A", "B") map testJobChainPath.orderKey

  private def jobChainElem(ordersAreRecoverable: Boolean) =
    <job_chain name="test" orders_recoverable={ordersAreRecoverable.toString}>
      <job_chain_node state="100" job="test"/>
      <job_chain_node.end state="end"/>
    </job_chain>
}
