package com.sos.scheduler.engine.tests.jira.js1159

import com.sos.scheduler.engine.common.scalautil.FileUtils.implicits._
import com.sos.scheduler.engine.common.system.Files.makeDirectory
import com.sos.scheduler.engine.common.time.ScalaJoda._
import com.sos.scheduler.engine.common.utils.FreeTcpPortFinder.{alternateTcpPortRange, findRandomFreeTcpPort}
import com.sos.scheduler.engine.data.job.JobPath
import com.sos.scheduler.engine.kernel.extrascheduler.ExtraScheduler
import com.sos.scheduler.engine.main.CppBinary
import com.sos.scheduler.engine.test.SchedulerTestUtils.runJobAndWaitForEnd
import com.sos.scheduler.engine.test.scala.ScalaSchedulerTest
import com.sos.scheduler.engine.test.scala.SchedulerTestImplicits._
import com.sos.scheduler.engine.tests.jira.js1159.JS1159IT._
import java.io.File
import org.junit.runner.RunWith
import org.scalatest.junit.JUnitRunner
import org.scalatest.{BeforeAndAfter, FreeSpec}
import scala.concurrent.Await

/**
 * JS-1159: Agent communication via HTTP.
 * @author Joacim Zschimmer
 */
@RunWith(classOf[JUnitRunner])
final class JS1159IT extends FreeSpec with ScalaSchedulerTest with BeforeAndAfter {

  private lazy val agentTcpPort = findRandomFreeTcpPort()
  private lazy val agentHttpPort = findRandomFreeTcpPort(alternateTcpPortRange)
  private lazy val extraScheduler = {
    val logDir = controller.environment.logDirectory / "agent"
    makeDirectory(logDir)
    val args = List(
      controller.cppBinaries.file(CppBinary.exeFilename).getPath,
      s"-sos.ini=${controller.environment.sosIniFile}",
      s"-ini=${controller.environment.iniFile}",
      s"-id=agent-$agentTcpPort",
      s"-roles=agent",
      s"-log-dir=$logDir",
      s"-log-level=debug9",
      s"-log=${logDir / "scheduler.log"}",
      s"-java-options=-XX:+CreateMinidumpOnCrash",
      s"-java-classpath=${System.getProperty("java.class.path")}",
      s"-job-java-classpath=${System.getProperty("java.class.path")}",
      s"-e",
      new File(controller.environment.configDirectory.getPath, "agent-scheduler.xml").getPath)
    new ExtraScheduler(args = args, env = Map(), httpPort = Some(agentHttpPort), tcpPort = Some(agentTcpPort))
  }

  "Start" in {
    extraScheduler.start()
    registerAutoCloseable(extraScheduler)
    scheduler executeXml <process_class name="agent-tcp" remote_scheduler={extraScheduler.tcpAddress.string}/>
    scheduler executeXml <process_class name="agent-http" remote_scheduler={extraScheduler.uri.toString}/>
    Await.result(extraScheduler.isActiveFuture, TestTimeout)
  }

  TestJobPaths foreach { jobPath ⇒
    jobPath.name.toString in {
      for (_ <- 1 to 3) {
        runJobAndWaitForEnd(jobPath)
      }
    }
  }
}

private object JS1159IT {
  private val TestJobPaths = List("/api-http", "/api-tcp", "/shell-http", "/shell-tcp") map JobPath.apply
}
