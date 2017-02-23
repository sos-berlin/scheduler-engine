package com.sos.scheduler.engine.tests.jira.js1354

import com.google.common.io.Files.touch
import com.sos.jobscheduler.common.scalautil.FileUtils.implicits._
import com.sos.jobscheduler.common.scalautil.xmls.ScalaXmls.implicits._
import com.sos.jobscheduler.common.time.ScalaTime._
import com.sos.jobscheduler.data.job.TaskId
import com.sos.scheduler.engine.data.jobchain.JobChainPath
import com.sos.scheduler.engine.data.order.OrderFinished
import com.sos.scheduler.engine.kernel.folder.FolderSubsystemClient
import com.sos.scheduler.engine.kernel.persistence.hibernate.HibernateVariableStore
import com.sos.scheduler.engine.kernel.persistence.hibernate.ScalaHibernate.transaction
import com.sos.scheduler.engine.test.EventBusTestFutures.implicits.RichEventBus
import com.sos.scheduler.engine.test.SchedulerTestUtils._
import com.sos.scheduler.engine.test.scalatest.ScalaSchedulerTest
import com.sos.scheduler.engine.tests.jira.js1354.JS1354IT._
import javax.persistence.EntityManagerFactory
import org.junit.runner.RunWith
import org.scalatest.FreeSpec
import org.scalatest.junit.JUnitRunner
import scala.concurrent.Future

/**
 * JS-1354, JS-1390, JS-1391 file_order_source should not lead to creation of needless task IDs
 *
 * @author Joacim Zschimmer
 */
@RunWith(classOf[JUnitRunner])
final class JS1354IT extends FreeSpec with ScalaSchedulerTest {

  private lazy val fileOrderDir = testEnvironment.newFileOrderSourceDirectory()
  private implicit lazy val entityManagerFactory: EntityManagerFactory = instance[EntityManagerFactory]

  "JS-1391 No needless task ID creation while the limit of a job chain with file_order_source is reached" - {
    "job_chain max_order=1" in {
      runFiles("TESTFILE-1a", "TESTFILE-1b") {
        testEnvironment.fileFromPath(TestJobChainPath).xml =
          <job_chain max_orders="1">
            <file_order_source directory={fileOrderDir.getPath} repeat="1"/>
            <job_chain_node state="100" job="/test-100"/>
            <job_chain_node state="200" job="/test-200"/>
          </job_chain>
        instance[FolderSubsystemClient].updateFolders()
      }
    }

    "Second files" in {
      runFiles("TESTFILE-2a", "TESTFILE-2b") {}
    }
  }

  "JS-1354 No needless task ID creation while waiting for a new file order" - {
    "job_chain max_order=unlimited" in {
      testEnvironment.fileFromPath(TestJobChainPath).xml =
        <job_chain>
          <file_order_source directory={fileOrderDir.getPath} repeat="1"/>
          <job_chain_node state="100" job="/test-100"/>
          <job_chain_node state="200" job="/test-200"/>
        </job_chain>
      instance[FolderSubsystemClient].updateFolders()
      runFiles("TESTFILE-1") {}
    }
  }

  "JS-1390 Don't fall asleep after first file order" - {
    "Second files" in {
      runFiles("TESTFILE-2a", "TESTFILE-2b") {}
    }

    "Third file" in {
      runFiles("TESTFILE-3") {}
    }
  }

  private def runFiles(names: String*)(body: ⇒ Unit): Unit = {
    val preId = nextTaskId
    val files = names map { o ⇒ fileOrderDir / o }
    val ordersFinished = Future.sequence(files map { f ⇒ eventBus.eventFuture[OrderFinished](TestJobChainPath orderKey f.getPath) })
    files foreach touch
    body
    awaitSuccess(ordersFinished)
    val postId = nextTaskId
    assert(postId == TaskId(preId.number + files.size * JobChainTaskCount))
    sleep(1.s)
    assert(nextTaskId == postId)
  }

  private def nextTaskId = transaction { implicit entityManager ⇒ instance[HibernateVariableStore].nextTaskId }
}

private object JS1354IT {
  private val TestJobChainPath = JobChainPath("/test")
  private val JobChainTaskCount = 2   // Job chain contains two jobs, for each a task will be started
}
