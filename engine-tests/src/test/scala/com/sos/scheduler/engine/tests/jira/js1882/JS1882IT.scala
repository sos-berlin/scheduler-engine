package com.sos.scheduler.engine.tests.jira.js1882

import com.sos.scheduler.engine.common.scalautil.Futures.implicits._
import com.sos.scheduler.engine.common.time.ScalaTime._
import com.sos.scheduler.engine.data.jobchain.{JobChainPath, NodeId}
import com.sos.scheduler.engine.test.SchedulerTestUtils.{runOrder, startOrder, writeConfigurationFile}
import com.sos.scheduler.engine.test.configuration.TestConfiguration
import com.sos.scheduler.engine.test.scalatest.ScalaSchedulerTest
import com.sos.scheduler.engine.tests.jira.js1882.JS1882IT._
import java.util.concurrent.TimeoutException
import org.junit.runner.RunWith
import org.scalatest.FreeSpec
import org.scalatest.junit.JUnitRunner
import scala.concurrent.Await
import scala.concurrent.duration._

@RunWith(classOf[JUnitRunner])
final class JS1882IT extends FreeSpec with ScalaSchedulerTest
{
  override protected lazy val testConfiguration = TestConfiguration(testClass = getClass,
    mainArguments = List("-log-level=info"),
    ignoreError = _ => true)

  "Job chain with wrong default process class" in {
    writeConfigurationFile(jobChainPath,
      <job_chain process_class="/MISSING-AGENT">
        <job_chain_node state="100" job="JOB"/>
        <job_chain_node.end state="END"/>
      </job_chain>)

    val whenFinished = startOrder(jobChainPath orderKey "ORDER-WITH-MISSING-AGENT").result
    intercept[TimeoutException] {
      Await.ready(whenFinished, 3.seconds)
    }
    assert(!whenFinished.isCompleted)
    scheduler executeXml <kill_task job="/JOB" id="3"/>  // kill_task required !!!
    whenFinished await 30.s
  }

  "Remove process class from job chain" in {
    writeConfigurationFile(jobChainPath,
      <job_chain>
        <job_chain_node state="100" job="JOB"/>
        <job_chain_node.end state="END"/>
      </job_chain>)

    val result = runOrder(jobChainPath orderKey "ORDER-WITHOUT-PROCESS-CLASS")
    assert(result.nodeId == NodeId("END"))
  }
}

private object JS1882IT
{
  private val jobChainPath = JobChainPath("/JOB-CHAIN")
}
