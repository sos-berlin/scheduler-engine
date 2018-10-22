package com.sos.scheduler.engine.tests.jira.js1789

import com.sos.scheduler.engine.common.scalautil.Logger
import com.sos.scheduler.engine.common.time.ScalaTime._
import com.sos.scheduler.engine.test.agent.AgentWithSchedulerTest
import com.sos.scheduler.engine.test.scalatest.ScalaSchedulerTest
import com.sos.scheduler.engine.tests.jira.js1789.JS1789IT._
import java.lang.management.ManagementFactory
import java.nio.file.Files
import java.nio.file.Files.delete
import org.junit.runner.RunWith
import org.scalatest.FreeSpec
import org.scalatest.junit.JUnitRunner
import scala.sys.process._

/** JS-1789 Universal Agent should not eat inotify user_instances - MANUAL TEST.
  * Run as single test:
  * mvn verify -pl :engine-tests -Dit.test=JS1789IT -DargLine=-DJS-1789
  * @author Joacim Zschimmer
  */
@RunWith(classOf[JUnitRunner])
final class JS1789IT extends FreeSpec with ScalaSchedulerTest with AgentWithSchedulerTest
{
  if (sys.props isDefinedAt "JS-1789") {
    "Universal Agent should not eat inotify user_instances" in {
      val dir = Files createTempDirectory "JS-1789-"
      scheduler executeXml
        <job_chain name="test" file_watching_process_class="/test-agent">
          <file_order_source directory={dir.toString}/>
          <job_chain_node state="JOB" job="/test"/>
          <file_order_sink state="DELETE" remove="true"/>
          <job_chain_node.end state="END"/>
        </job_chain>
      val pid = ManagementFactory.getRuntimeMXBean.getName takeWhile (_ != '@')  // http://stackoverflow.com/questions/35842
      while (true) {
        val handles = exec(s"ls -l /proc/$pid/fd")
        val inotifyHandles = handles.filter(_ contains "inotify")
        logger.info("### " + handles.length + " open files")
        logger.info(inotifyHandles.map("### " + _).mkString("\n"))
        assert(inotifyHandles.size <= 1)
        sleep(1.s)
      }
      delete(dir)
    }
  }

  private def exec(command: String): Vector[String] = {
    logger.info(s"### $command")
    var output, error = Vector.empty[String]
    command.run(
      new ProcessIO(
        stdin  ⇒ stdin.close(),
        stdout ⇒ {
          output = scala.io.Source.fromInputStream(stdout).getLines.toVector
          stdout.close()
        },
        stderr ⇒ {
          error = scala.io.Source.fromInputStream(stderr).getLines.toVector
          stderr.close()
        }))
      .exitValue()/*wait for process*/
    // Ignore exit code
    error ++ output
  }
}

object JS1789IT {
  private val logger = Logger(getClass)
}
