package com.sos.scheduler.engine.tests.jira.js1655

import com.sos.scheduler.engine.common.scalautil.Closers.implicits._
import com.sos.scheduler.engine.common.scalautil.FileUtils.implicits.RichFile
import com.sos.scheduler.engine.common.scalautil.Futures.awaitResult
import com.sos.scheduler.engine.common.system.Files.makeDirectory
import com.sos.scheduler.engine.common.time.ScalaJoda._
import com.sos.scheduler.engine.common.utils.FreeTcpPortFinder.findRandomFreeTcpPort
import com.sos.scheduler.engine.data.job.JobPath
import com.sos.scheduler.engine.kernel.extrascheduler.ExtraScheduler
import com.sos.scheduler.engine.main.CppBinary
import com.sos.scheduler.engine.test.SchedulerTestUtils._
import com.sos.scheduler.engine.test.scalatest.ScalaSchedulerTest
import org.junit.runner.RunWith
import org.scalatest.FreeSpec
import org.scalatest.junit.JUnitRunner

/**
  * @author Joacim Zschimmer
  */
@RunWith(classOf[JUnitRunner])
final class JS1655IT extends FreeSpec with ScalaSchedulerTest {

  private lazy val agentHttpPort = findRandomFreeTcpPort()
  private lazy val extraScheduler = {
    val logDir = controller.environment.logDirectory / "agent"
    makeDirectory(logDir)
    val args = List(
      controller.cppBinaries.file(CppBinary.exeFilename).getPath,
      s"-sos.ini=${controller.environment.sosIniFile}",
      s"-ini=${controller.environment.iniFile}",
      s"-roles=agent",
      s"-log-dir=$logDir",
      s"-log-level=debug9",
      s"-log=${logDir / "scheduler.log"}",
      s"-java-classpath=${System.getProperty("java.class.path")}",
      s"-job-java-classpath=${System.getProperty("java.class.path")}",
      (controller.environment.configDirectory / "agent-scheduler.xml").getPath)
    new ExtraScheduler(args = args, httpPort = Some(agentHttpPort)).closeWithCloser
  }

  "Start" in {
    extraScheduler.start()
    scheduler executeXml <process_class name="agent-http" remote_scheduler={extraScheduler.uri}/>
    awaitResult(extraScheduler.activatedFuture, TestTimeout)
    scheduler executeXml <start_job job="/test"/>
  }

  "agent_url" in {
    val taskResult = runJob(JobPath("/test"))
    assert(taskResult.logString contains s"AGENT_URL=http://127.0.0.1:$agentHttpPort")
  }

  "Command rotate_logs" in {
    def isKnownLog = testEnvironment.schedulerLog.contentString contains "INSERT into SCHEDULER_TASKS"
    assert(isKnownLog)
    scheduler executeXml <modify_spooler cmd="rotate_logs"/>
    assert(!isKnownLog)
  }
}
