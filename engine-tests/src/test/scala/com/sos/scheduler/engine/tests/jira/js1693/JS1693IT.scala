package com.sos.scheduler.engine.tests.jira.js1693

import com.sos.jobscheduler.common.system.OperatingSystem.isWindows
import com.sos.scheduler.engine.data.jobchain.JobChainPath
import com.sos.scheduler.engine.test.SchedulerTestUtils._
import com.sos.scheduler.engine.test.scalatest.ScalaSchedulerTest
import org.junit.runner.RunWith
import org.scalatest.FreeSpec
import org.scalatest.junit.JUnitRunner

/**
  * @author Joacim Zschimmer
  */
@RunWith(classOf[JUnitRunner])
final class JS1693IT extends FreeSpec with ScalaSchedulerTest {

  if (isWindows)
  "Environment variables in order variables" in {
    val variables = runOrder(JobChainPath("/test") orderKey "1").variables
    println(variables)
    assert(variables("path") == "/" + sys.env.getOrElse("PATH", sys.env("Path")) + "/")
  }
}
