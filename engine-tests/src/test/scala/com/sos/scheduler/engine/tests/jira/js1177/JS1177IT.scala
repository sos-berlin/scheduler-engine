package com.sos.scheduler.engine.tests.jira.js1177

import com.sos.jobscheduler.common.time.ScalaTime._
import com.sos.jobscheduler.common.time.WaitForCondition.waitForCondition
import com.sos.scheduler.engine.data.job.JobPath
import com.sos.scheduler.engine.data.xmlcommands.StartJobCommand
import com.sos.scheduler.engine.test.ClusterTest
import com.sos.scheduler.engine.test.SchedulerTestUtils._
import com.sos.scheduler.engine.tests.jira.js1177.JS1177IT._
import javax.persistence.EntityManagerFactory
import org.junit.runner.RunWith
import org.scalatest.FreeSpec
import org.scalatest.Matchers._
import org.scalatest.junit.JUnitRunner
import scala.collection.immutable

/**
 * @author Joacim Zschimmer
 */
@RunWith(classOf[JUnitRunner])
final class JS1177IT extends FreeSpec with ClusterTest {

  protected def clusterMemberCount = 2
  private lazy val otherScheduler = otherSchedulers(0)
  private implicit lazy val entityManagerFactory = instance[EntityManagerFactory]

  "Command show_history shows job starts of all cluster members" in {
    awaitSuccess(otherScheduler.postCommand(StartJobCommand(TestJobPath)))
    runJob(TestJobPath)
    val showHistoryContainsBothJobStarts =
      waitForCondition(TestTimeout, 500.ms) {
        val ports = showHistoryMemberTcpPorts()
        ports.size should (be >= 1 and be <= 2)
        assert(ports contains ownHttpPort)
        ports.size == 2 && {
          assert(ports.size == 2, "- Two job start history entries expected")
          assert(ports.toSet == Set(ownHttpPort, otherPorts(0)))
          true
        }
      }
    assert(showHistoryContainsBothJobStarts)
  }

  /**
   * @return the cluster member tcp ports of the started jobs with same path, returned by &lt;show_history>.
   */
  private def showHistoryMemberTcpPorts(): immutable.Seq[Int] = {
    val historyEntries = (scheduler executeXml <show_history job={TestJobPath.string}/>).answer \ "history" \ "history.entry"
    val historyMemberIds = historyEntries map { o ⇒ (o \@ "cluster_member_id").toString }
    historyMemberIds map { case ClusterMemberIdRegex(_, _, tcpPort) ⇒ tcpPort.toInt }
  }
}

private object JS1177IT {
  private val ClusterMemberIdRegex = """^(.*)/(.*):([0-9]+)$""".r
  private val TestJobPath = JobPath("/test")
}
