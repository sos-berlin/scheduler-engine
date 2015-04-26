package com.sos.scheduler.engine.tests.jira.js1357

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
import com.sos.scheduler.engine.tests.jira.js1357.JS1357IT._
import javax.persistence.EntityManagerFactory
import org.junit.runner.RunWith
import org.scalatest.FreeSpec
import org.scalatest.junit.JUnitRunner

/**
 * JS-1357 Avoid unneccessary creation of new ids for table scheduler_history.
 *
 * @author Joacim Zschimmer
 */
@RunWith(classOf[JUnitRunner])
final class JS1357IT extends FreeSpec with ScalaSchedulerTest {

  private lazy val fileOrderDir = testEnvironment.newFileOrderSourceDirectory()
  private implicit lazy val entityManagerFactory: EntityManagerFactory = instance[EntityManagerFactory]

  "Database variable for next task ID is not incremented while JobScheduler idles" in {
    val preId = nextTaskId

    testEnvironment.fileFromPath(TestJobChainPath).xml =
      <job_chain max_orders="1">
        <file_order_source directory={fileOrderDir.getPath}/>
        <job_chain_node state="100" job="/test"/>
        <file_order_sink state="SINK" remove="yes"/>
        <job_chain_node.end state="END"/>
      </job_chain>
    instance[FolderSubsystem].updateFolders()

    val file = fileOrderDir / "TESTFILE"
    eventBus.awaitingKeyedEvent[OrderFinishedEvent](TestJobChainPath orderKey file.getPath) {
      touch(file)   // Under Unix, detected via 10s polling
    }

    val postId = nextTaskId
    assert(postId == TaskId(preId.value + JobChainTaskCount))
    sleep(15.s)
    assert(nextTaskId == postId)
  }

  private def nextTaskId = transaction { implicit entityManager ⇒ instance[HibernateVariableStore].nextTaskId }
}

private object JS1357IT {
  private val TestJobChainPath = JobChainPath("/test")
  private val JobChainTaskCount = 2   // Job chain contains two jobs (nodes 100 and SINK), for each a task has been started
}
