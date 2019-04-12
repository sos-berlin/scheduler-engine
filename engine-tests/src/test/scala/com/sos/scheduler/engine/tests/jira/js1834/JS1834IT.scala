package com.sos.scheduler.engine.tests.jira.js1834

import com.sos.scheduler.engine.common.scalautil.AutoClosing.autoClosing
import com.sos.scheduler.engine.common.utils.FreeTcpPortFinder.findRandomFreeTcpPort
import com.sos.scheduler.engine.data.event.{Event, KeyedEvent}
import com.sos.scheduler.engine.data.filebased.FileBasedEvent
import com.sos.scheduler.engine.data.job.TaskId
import com.sos.scheduler.engine.data.jobchain.{JobChainPath, NodeId}
import com.sos.scheduler.engine.data.order.{OrderEvent, OrderFinished, OrderId, OrderNodeChanged, OrderNodeTransition, OrderResumed, OrderStarted, OrderStepEnded, OrderStepStarted, OrderSuspended}
import com.sos.scheduler.engine.data.xmlcommands.ModifyOrderCommand
import com.sos.scheduler.engine.kernel.settings.{CppSettingName, CppSettings}
import com.sos.scheduler.engine.test.EventBusTestFutures.implicits.RichEventBus
import com.sos.scheduler.engine.test.SchedulerTestUtils.order
import com.sos.scheduler.engine.test.configuration.TestConfiguration
import com.sos.scheduler.engine.test.database.H2DatabaseServer
import com.sos.scheduler.engine.test.{ImplicitTimeout, ProvidesTestEnvironment, TestSchedulerController}
import org.junit.runner.RunWith
import org.scalatest.FreeSpec
import org.scalatest.junit.JUnitRunner

/**
  * @author Joacim Zschimmer
  */
@RunWith(classOf[JUnitRunner])
final class JS1834IT extends FreeSpec
{
  private implicit val implicitTimeout: ImplicitTimeout = TestSchedulerController.implicits.Timeout

  private lazy val databaseTcpPort = findRandomFreeTcpPort()

  private val jobChainPath = JobChainPath("/test")
  private val orderKey = jobChainPath orderKey OrderId("ORDER")

  "Started permanent order gets its original variables after JobScheduler restart and order termination" in {
    lazy val databaseConfiguration: H2DatabaseServer.Configuration = new H2DatabaseServer.Configuration {
      def directory = envProvider.testEnvironment.databaseDirectory
      def tcpPort = databaseTcpPort
    }
    lazy val testConfiguration = TestConfiguration(getClass,
      database = Some(databaseConfiguration),
      cppSettings = CppSettings.TestMap + (CppSettingName.alwaysCreateDatabaseTables -> false.toString))
    lazy val envProvider: ProvidesTestEnvironment = ProvidesTestEnvironment(testConfiguration)
    autoClosing(envProvider) { _ =>
      val databaseServer = new H2DatabaseServer(databaseConfiguration)
      databaseServer.start()

      envProvider.runScheduler() { implicit controller =>
        import controller.{eventBus, scheduler}

        assert(order(orderKey).variables == Map("TEST" -> "INITIAL"))

        // FIRST RUN WITHOUT JOBSCHEDULER RESTART

        eventBus.awaiting[OrderSuspended.type](orderKey) {
          scheduler executeXml ModifyOrderCommand(orderKey, at = Some(ModifyOrderCommand.NowAt))
        }
        assert(order(orderKey).variables == Map("TEST" -> "INITIAL-CHANGED"))

        eventBus.awaiting[OrderFinished](orderKey) {
          scheduler executeXml ModifyOrderCommand(orderKey, suspended = Some(false))
        }
        assert(order(orderKey).nodeId == NodeId("100"))
        assert(order(orderKey).variables == Map("TEST" -> "INITIAL"))

        // SECOND RUN WITH JOBSCHEDULER RESTART
        eventBus.awaiting[OrderSuspended.type](orderKey) {
          scheduler executeXml ModifyOrderCommand(orderKey, at = Some(ModifyOrderCommand.NowAt))
        }
        assert(order(orderKey).variables == Map("TEST" -> "INITIAL-CHANGED"))
      }

      envProvider.runScheduler() { implicit controller =>
        import controller.{eventBus, scheduler}
        val eventPipe = controller.newEventPipe()

        assert(order(orderKey).variables == Map("TEST" -> "INITIAL-CHANGED"))

        eventBus.awaiting[OrderFinished](orderKey) {
          scheduler executeXml ModifyOrderCommand(orderKey, suspended = Some(false))
        }
        assert(order(orderKey).nodeId == NodeId("100"))
        assert(order(orderKey).variables == Map("TEST" -> "INITIAL"))   // THE BUG RETURNED "INITIAL-CHANGED"

        eventBus.awaiting[OrderSuspended.type](orderKey) {
          scheduler executeXml ModifyOrderCommand(orderKey, at = Some(ModifyOrderCommand.NowAt))
        }
        assert(order(orderKey).variables == Map("TEST" -> "INITIAL-CHANGED"))

        scheduler.executeCallQueue()
        assert(eventPipe.queued[Event].collect { case o @ KeyedEvent(_, _: OrderEvent | _: FileBasedEvent) => o } ==
          Vector(
            orderKey <-: OrderResumed,
            orderKey <-: OrderStepStarted(NodeId("200"), TaskId.First + 4),
            orderKey <-: OrderStepEnded(OrderNodeTransition.Success),
            orderKey <-: OrderNodeChanged(NodeId("END"), NodeId("200")),
            orderKey <-: OrderFinished(NodeId("END")),
            orderKey <-: OrderNodeChanged(NodeId("100"), NodeId("END")),

            orderKey <-: OrderStarted,
            orderKey <-: OrderStepStarted(NodeId("100"), TaskId.First + 5),
            orderKey <-: OrderStepEnded(OrderNodeTransition.Success),
            orderKey <-: OrderSuspended,
            orderKey <-: OrderNodeChanged(NodeId("200"), NodeId("100"))))

        eventBus.awaiting[OrderFinished](orderKey) {
          scheduler executeXml ModifyOrderCommand(orderKey, suspended = Some(false))
        }
        assert(order(orderKey).nodeId == NodeId("100"))
        assert(order(orderKey).variables == Map("TEST" -> "INITIAL"))

        // SECOND RUN WITH JOBSCHEDULER RESTART
        eventBus.awaiting[OrderSuspended.type](orderKey) {
          scheduler executeXml ModifyOrderCommand(orderKey, at = Some(ModifyOrderCommand.NowAt))
        }
        assert(order(orderKey).variables == Map("TEST" -> "INITIAL-CHANGED"))
      }

      databaseServer.close()
    }
  }
}
