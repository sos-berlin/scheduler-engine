package com.sos.scheduler.engine.tests.jira.js1566

import com.sos.jobscheduler.common.scalautil.Logger
import com.sos.jobscheduler.common.time.ScalaTime._
import com.sos.scheduler.engine.data.jobchain.JobChainPath
import com.sos.scheduler.engine.test.ImplicitTimeout
import com.sos.scheduler.engine.test.SchedulerTestUtils._
import com.sos.scheduler.engine.test.scalatest.ScalaSchedulerTest
import com.sos.scheduler.engine.tests.jira.js1566.JS1566IT._
import java.time.{Duration, Instant}
import org.junit.runner.RunWith
import org.scalatest.FreeSpec
import org.scalatest.junit.JUnitRunner
import scala.collection.mutable

/**
  * @author Joacim Zschimmer
  */
@RunWith(classOf[JUnitRunner])
final class JS1566IT extends FreeSpec with ScalaSchedulerTest {

  override protected implicit val implicitTimeout = ImplicitTimeout(60.s)

  "Performance impact of a big job chain (no test)" in {
    val times = mutable.Buffer[(String, Duration)]()
    times += s"warm-up" → measureTime {
      runOrder(LittleJobChainPath orderKey "1")
    }
    for (_ ← 1 to 3) {
      times += s"order runs through little job chain" → measureTime {
        runOrder(LittleJobChainPath orderKey "1")
      }
    }
    times += s"adding job chain with $NodeCount nodes" → measureTime {
      deleteAndWriteConfigurationFile(BigJobChainPath, makeJobChain(NodeNames))
    }
    for (_ ← 1 to 3) {
      times += s"order runs through little job chain" → measureTime {
        runOrder(LittleJobChainPath orderKey "1")
      }
    }
    times += s"skip first $SkipCount nodes" → measureTime {
      for (i ← NodeNames take SkipCount) {
        scheduler executeXml <job_chain_node.modify job_chain={BigJobChainPath.string} state={s"$i"} action="next_state"/>
      }
    }
    for ((caption, duration) ← times) logger.info(f"$caption%-40s ${duration.pretty}")
  }
}

private object JS1566IT {
  private val logger = Logger(getClass)
  private val SkipCount = 5
  private val NodeCount = 1000   // Performance impact is about O(n²) or O(n³)
  private val NodeNames = 1 to NodeCount
  private val BigJobChainPath = JobChainPath("/big")
  private val LittleJobChainPath = JobChainPath("/test-little")

  private def makeJobChain(names: Range): xml.Elem =
    <job_chain>{
        for (i ← names) yield <job_chain_node state={s"$i"} next_state={s"${i+1}"} job="/test-api"/>
      }
      <job_chain_node.end state={s"${names.last + 1}"}/>
    </job_chain>

  private def measureTime(body: ⇒ Unit): Duration = {
    val t = Instant.now
    body
    Instant.now - t
  }
}
