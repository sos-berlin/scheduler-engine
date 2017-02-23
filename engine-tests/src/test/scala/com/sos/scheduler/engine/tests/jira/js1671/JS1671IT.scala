package com.sos.scheduler.engine.tests.jira.js1671

import com.sos.jobscheduler.common.utils.FreeTcpPortFinder._
import com.sos.scheduler.engine.data.job.JobPath
import com.sos.scheduler.engine.test.SchedulerTestUtils._
import com.sos.scheduler.engine.test.configuration.TestConfiguration
import com.sos.scheduler.engine.test.scalatest.ScalaSchedulerTest
import org.junit.runner.RunWith
import org.scalatest.FreeSpec
import org.scalatest.junit.JUnitRunner

/**
  * JS-1671 Webs service access token for jobs.
  * @author Joacim Zschimmer
  */
@RunWith(classOf[JUnitRunner])
final class JS1671IT extends FreeSpec with ScalaSchedulerTest {

  private lazy val httpPort = findRandomFreeTcpPort()
  protected override lazy val testConfiguration = TestConfiguration(getClass,
    mainArguments = List(s"-http-port=$httpPort"))

  "With access token, access is granted" in {
    val result = runJob(JobPath("/WithAccessTokenJob"))
    assert(result.returnCode.isSuccess)
  }

  "Without access token, access is rejected" in {
    val result = runJob(JobPath("/WithoutAccessTokenJob"))
    assert(result.returnCode.isSuccess)
  }
}
