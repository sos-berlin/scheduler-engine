package com.sos.scheduler.engine.tests.jira.js1580

import com.sos.jobscheduler.common.scalautil.Futures.implicits._
import com.sos.jobscheduler.data.agent.AgentAddress
import com.sos.scheduler.engine.data.job.JobPath
import com.sos.scheduler.engine.kernel.persistence.hibernate.RichEntityManager.toRichEntityManager
import com.sos.scheduler.engine.kernel.persistence.hibernate.ScalaHibernate._
import com.sos.scheduler.engine.persistence.entities.TaskHistoryEntity
import com.sos.scheduler.engine.test.SchedulerTestUtils._
import com.sos.scheduler.engine.test.TestSchedulerController
import com.sos.scheduler.engine.test.agent.AgentWithSchedulerTest
import com.sos.scheduler.engine.test.scalatest.ScalaSchedulerTest
import javax.persistence.EntityManagerFactory
import org.junit.runner.RunWith
import org.scalatest.FreeSpec
import org.scalatest.junit.JUnitRunner
import scala.concurrent.Future

/**
  * @author Joacim Zschimmer
  */
@RunWith(classOf[JUnitRunner])
final class JS1580IT extends FreeSpec with ScalaSchedulerTest with AgentWithSchedulerTest {

  "New database column agent_url" in {
    val setting = List(JobPath("/test") → None, JobPath("/test-agent") → Some(agentUri))
    val futures: List[Future[Boolean]] =
      for ((jobPath, expectedAgentUri) ← setting;
           run = startJob(jobPath)) yield
        for (_ ← run.started) yield
          transaction(entityManagerFactory) { em ⇒
            val e = em.fetchOption[TaskHistoryEntity]("select t from TaskHistoryEntity t where t.id = :taskId",
              List("taskId" → run.taskId.number))
            assert((Option(e.get.agentUrl) map AgentAddress.apply) == expectedAgentUri)
            true
          }
    futures await TestTimeout
  }

  private def entityManagerFactory(implicit controller: TestSchedulerController) =
    controller.instance[EntityManagerFactory]
}
