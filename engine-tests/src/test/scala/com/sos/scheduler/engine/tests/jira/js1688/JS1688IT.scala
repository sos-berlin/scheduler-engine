package com.sos.scheduler.engine.tests.jira.js1688

import com.sos.scheduler.engine.common.scalautil.FileUtils.implicits._
import com.sos.scheduler.engine.common.time.ScalaTime._
import com.sos.scheduler.engine.data.job.{JobPath, TaskId}
import com.sos.scheduler.engine.data.jobchain.JobChainPath
import com.sos.scheduler.engine.data.xmlcommands.ModifyJobCommand
import com.sos.scheduler.engine.kernel.scheduler.SchedulerConstants
import com.sos.scheduler.engine.test.SchedulerTestUtils._
import com.sos.scheduler.engine.test.scalatest.ScalaSchedulerTest
import com.sos.scheduler.engine.tests.jira.js1688.JS1688IT._
import org.junit.runner.RunWith
import org.scalatest.FreeSpec
import org.scalatest.junit.JUnitRunner

/**
  * @author Joacim Zschimmer
  */
@RunWith(classOf[JUnitRunner])
final class JS1688IT extends FreeSpec with ScalaSchedulerTest {

  "JS-1688" in {
    val dir = testEnvironment.newFileOrderSourceDirectory()
    scheduler executeXml ModifyJobCommand(TestJobPath, cmd = Some(ModifyJobCommand.Cmd.Stop))
    writeConfigurationFile(TestJobChainPath,
      <job_chain>
        <file_order_source directory={dir.toString}/>
        <job_chain_node state="100" job="/test"/>
        <job_chain_node.end state="END"/>
      </job_chain>)
    scheduler executeXml <job_chain.modify job_chain={TestJobChainPath.string} state="stopped"/>
    scheduler executeXml ModifyJobCommand(TestJobPath, cmd = Some(ModifyJobCommand.Cmd.Unstop))
    (dir / "test").contentString = ""
    sleep(1.s)
    val taskId = runJob(TestJobPath).taskId
    //println(testEnvironment.schedulerLog.contentString)
    assert(taskId == TaskId(SchedulerConstants.taskIdOffset))
  }
}

object JS1688IT {
  private val TestJobChainPath = JobChainPath("/test")
  private val TestJobPath = JobPath("/test")
}
