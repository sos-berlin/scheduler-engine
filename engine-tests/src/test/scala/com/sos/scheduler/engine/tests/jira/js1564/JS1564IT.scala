package com.sos.scheduler.engine.tests.jira.js1564

import com.sos.jobscheduler.common.scalautil.Futures.implicits.RichFutures
import com.sos.jobscheduler.common.time.ScalaTime._
import com.sos.scheduler.engine.data.jobchain.JobChainPath
import com.sos.scheduler.engine.test.SchedulerTestUtils._
import com.sos.scheduler.engine.test.configuration.TestConfiguration
import com.sos.scheduler.engine.test.scalatest.ScalaSchedulerTest
import org.junit.runner.RunWith
import org.scalatest.FreeSpec
import org.scalatest.junit.JUnitRunner
import scala.collection.mutable

/**
  * JS-1564 JobScheduler Active Cluster enqueued tasks are not started once &lt;process_class max_process="..."> limit is reached.
  *
  * @author Joacim Zschimmer
  */
@RunWith(classOf[JUnitRunner])
final class JS1564IT extends FreeSpec with ScalaSchedulerTest {

  override protected lazy val testConfiguration = TestConfiguration(getClass, mainArguments = List("-distributed-orders"))

  "Running multiple orders in a one-process process class" in {
    val runs = mutable.Buffer[OrderRun]()
    runs += startOrder(JobChainPath("/test-a") orderKey "1")
    sleep(500.ms)
    runs += startOrder(JobChainPath("/test-a") orderKey "2")
    runs += startOrder(JobChainPath("/test-a") orderKey "3")
    runs += startOrder(JobChainPath("/test-b") orderKey "4")
    runs += startOrder(JobChainPath("/test-b") orderKey "5")
    runs map { _.finished } await TestTimeout
  }
}
