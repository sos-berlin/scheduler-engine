package com.sos.scheduler.engine.tests.jira.js1354

import com.google.common.io.Files.touch
import com.sos.scheduler.engine.common.scalautil.FileUtils.implicits._
import com.sos.scheduler.engine.common.scalautil.xmls.ScalaXmls.implicits._
import com.sos.scheduler.engine.common.time.ScalaJoda._
import com.sos.scheduler.engine.data.job.TaskId
import com.sos.scheduler.engine.data.jobchain.JobChainPath
import com.sos.scheduler.engine.data.order.OrderFinishedEvent
import com.sos.scheduler.engine.kernel.folder.FolderSubsystem
import com.sos.scheduler.engine.kernel.persistence.hibernate.HibernateVariableStore
import com.sos.scheduler.engine.kernel.persistence.hibernate.ScalaHibernate.transaction
import com.sos.scheduler.engine.test.EventBusTestFutures.implicits.RichEventBus
import com.sos.scheduler.engine.test.scalatest.ScalaSchedulerTest
import com.sos.scheduler.engine.tests.jira.js1354.JS1354IT._
import javax.persistence.EntityManagerFactory
import org.junit.runner.RunWith
import org.scalatest.FreeSpec
import org.scalatest.junit.JUnitRunner

/**
 * JS-1354 Avoid unneccessary creation of new ids for table scheduler_history.
 *
 * @author Joacim Zschimmer
 */
@RunWith(classOf[JUnitRunner])
final class JS1354IT extends FreeSpec with ScalaSchedulerTest {

  private lazy val fileOrderDir = testEnvironment.newFileOrderSourceDirectory()
  private implicit lazy val entityManagerFactory: EntityManagerFactory = instance[EntityManagerFactory]

  "Database variable for next task ID is not incremented while JobScheduler idles" in {
    val preId = nextTaskId

    testEnvironment.fileFromPath(TestJobChainPath).xml =
      <job_chain>
        <file_order_source directory={fileOrderDir.getPath} repeat="1"/>
        <job_chain_node state="100" job="/test-100"/>
        <job_chain_node state="200" job="/test-200"/>
        <file_order_sink state="SINK" remove="yes"/>
        <job_chain_node.end state="END"/>
      </job_chain>
    instance[FolderSubsystem].updateFolders()

    val file = fileOrderDir / "TESTFILE"
    eventBus.awaitingKeyedEvent[OrderFinishedEvent](TestJobChainPath orderKey file.getPath) {
      touch(file)
    }

    val postId = nextTaskId
    assert(postId == TaskId(preId.value + JobChainTaskCount))
    sleep(3.s)
    assert(nextTaskId == postId)
  }

  private def nextTaskId = transaction { implicit entityManager ⇒ instance[HibernateVariableStore].nextTaskId }
}

private object JS1354IT {
  private val TestJobChainPath = JobChainPath("/test")
  private val JobChainTaskCount = 3   // Job chain contains three jobs (nodes 100, 200 and SINK), for each a task is being started
}
