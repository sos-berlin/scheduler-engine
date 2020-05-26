package com.sos.scheduler.engine.tests.jira.js1879

import com.sos.scheduler.engine.data.jobchain.JobChainPath
import com.sos.scheduler.engine.data.order.OrderFinished
import com.sos.scheduler.engine.kernel.settings.{CppSettingName, CppSettings}
import com.sos.scheduler.engine.test.configuration.TestConfiguration
import com.sos.scheduler.engine.test.{ProvidesTestEnvironment, TestSchedulerController}
import com.sos.scheduler.engine.tests.jira.js1879.JS1879IT._
import org.junit.runner.RunWith
import org.scalatest.junit.JUnitRunner
import org.scalatest.{BeforeAndAfterAll, FreeSpec}

@RunWith(classOf[JUnitRunner])
final class JS1879IT extends FreeSpec with BeforeAndAfterAll with ProvidesTestEnvironment
{
  protected lazy val testConfiguration =
    TestConfiguration(
      testClass = getClass,
      cppSettings = CppSettings.TestMap + (CppSettingName.alwaysCreateDatabaseTables -> false.toString))

  override def afterAll() = {
    closer.close()
    super.afterAll()
  }

  "First run" in {
    runScheduler() { implicit controller =>
      runMyOrder()
    }
  }

  "Restart should work with relative paths, too" in {
    runScheduler() { implicit controller =>
      runMyOrder()
    }
  }

  private def runMyOrder()(implicit controller: TestSchedulerController): Unit = {
    controller.withEventPipe { eventPipe =>
      controller.scheduler executeXml <modify_order job_chain={orderKey.jobChainPath.string} order={orderKey.id.string} at="now"/>
      eventPipe.next[OrderFinished](orderKey)
    }
  }
}

private object JS1879IT
{
  private val orderKey = JobChainPath("/orders/x/y/TEST") orderKey "ORDER"
}
