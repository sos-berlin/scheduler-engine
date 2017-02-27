package com.sos.scheduler.engine.tests.jira.js1621

import com.sos.jobscheduler.common.scalautil.Logger
import com.sos.jobscheduler.common.time.ScalaTime._
import com.sos.jobscheduler.common.time.Stopwatch.itemsPerSecondString
import com.sos.scheduler.engine.data.job.JobPath
import com.sos.scheduler.engine.test.ImplicitTimeout
import com.sos.scheduler.engine.test.SchedulerTestUtils._
import com.sos.scheduler.engine.test.agent.AgentWithSchedulerTest
import com.sos.scheduler.engine.test.scalatest.ScalaSchedulerTest
import com.sos.scheduler.engine.tests.jira.js1621.JS1621IT._
import org.junit.runner.RunWith
import org.scalatest.FreeSpec
import org.scalatest.junit.JUnitRunner

/**
  * @author Joacim Zschimmer
  */
@RunWith(classOf[JUnitRunner])
final class JS1621IT extends FreeSpec with ScalaSchedulerTest with AgentWithSchedulerTest {

  implicit override protected val implicitTimeout = ImplicitTimeout(500.s)
  private val n = 10000

  s"Meter time for $n lines of stdout (do not fail)" in {
    runJob(JobPath("/test"), variables = Map("n" → 1.toString))  // warm-up

    val result = runJob(JobPath("/test"), variables = Map("n" → n.toString))
    logger.info(itemsPerSecondString(result.duration, n, "lines"))

    val stdoutIterator = result.logString.lines collect { case StdoutRegex(line) ⇒ line }
    for (i ← 1 to n) assert(stdoutIterator.next() == stdoutLine(i))
    assert(!stdoutIterator.hasNext)
  }
}

private object JS1621IT {
  private val logger = Logger(getClass)
  private val StdoutRegex = """.*\[stdout] (.*)""".r
  private def stdoutLine(i: Int) =
    s"....'....1....'....2....'....3....'....4....'....5....'....6....'....7....'....8....'....9....'....0 $i"
}
