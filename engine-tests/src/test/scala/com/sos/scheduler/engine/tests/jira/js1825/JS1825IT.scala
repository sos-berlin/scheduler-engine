package com.sos.scheduler.engine.tests.jira.js1825

import com.google.common.io.Files.touch
import com.sos.scheduler.engine.common.scalautil.AutoClosing.autoClosing
import com.sos.scheduler.engine.common.scalautil.FileUtils.implicits._
import com.sos.scheduler.engine.common.scalautil.Futures.implicits._
import com.sos.scheduler.engine.common.time.ScalaTime._
import com.sos.scheduler.engine.data.jobchain.JobChainPath
import com.sos.scheduler.engine.data.log.ErrorLogEvent
import com.sos.scheduler.engine.data.message.MessageCode
import com.sos.scheduler.engine.data.order.OrderStepStartedEvent
import com.sos.scheduler.engine.kernel.settings.{CppSettingName, CppSettings}
import com.sos.scheduler.engine.test.EventBusTestFutures.implicits.RichEventBus
import com.sos.scheduler.engine.test.configuration.TestConfiguration
import com.sos.scheduler.engine.test.{ImplicitTimeout, ProvidesTestEnvironment, TestSchedulerController}
import java.nio.file.Files.{createDirectories, exists}
import org.junit.runner.RunWith
import org.scalatest.FreeSpec
import org.scalatest.junit.JUnitRunner

@RunWith(classOf[JUnitRunner])
final class JS1825IT extends FreeSpec
{
  private lazy val testConfiguration =
    TestConfiguration(
      testClass = getClass,
      cppSettings = CppSettings.TestMap + (CppSettingName.alwaysCreateDatabaseTables -> false.toString),
      logCategories = "scheduler TEST jdbc all",
      ignoreError = Set(MessageCode("SCHEDULER-340")))
  private implicit val timeout = ImplicitTimeout(99.s)

  "TEST" in {
    autoClosing(ProvidesTestEnvironment(testConfiguration)) { envProvider =>
      envProvider.testEnvironment.prepare()
      val directory = envProvider.testEnvironment.directory / "tmp" / "files"
      createDirectories(directory)
      val jobChainXml =
        <job_chain name="TEST">
          <file_order_source directory={directory.toString}/>
          <job_chain_node state="100" job="/test"/>
          <job_chain_node.end state="END"/>
        </job_chain>
      val file = directory / "TEST"
      val orderKey = JobChainPath("/TEST") orderKey file.toString
      def isOnBlacklist(implicit controller: TestSchedulerController) =
        (controller.scheduler.executeXml(<show_order job_chain="/TEST" order={orderKey.id.string}/>)
          .answer \ "order" \ "@on_blacklist").toString == "yes"

      envProvider.runScheduler() { implicit controller =>
        controller.scheduler executeXml jobChainXml
        controller.eventBus.awaitingEvent[ErrorLogEvent](_.codeOption contains MessageCode("SCHEDULER-340")) {  // Has been set on blacklist
          touch(file)
        }
        assert(exists(file))
        assert(isOnBlacklist)
      }

      envProvider.runScheduler() { implicit controller =>
        val orderStarted = controller.eventBus.eventFuture[OrderStepStartedEvent](_.orderKey == orderKey)
        controller.scheduler executeXml jobChainXml
        assert(exists(file))
        assert(isOnBlacklist)
        intercept[java.util.concurrent.TimeoutException] {
          orderStarted.await(2.s)
          fail("After JobScheduler restart, blacklisted order has been started")
        }
        assert(isOnBlacklist)
      }
    }
  }
}
