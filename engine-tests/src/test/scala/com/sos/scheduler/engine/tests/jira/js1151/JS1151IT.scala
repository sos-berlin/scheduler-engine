package com.sos.scheduler.engine.tests.jira.js1151

import com.sos.scheduler.engine.common.scalautil.AutoClosing.autoClosing
import com.sos.scheduler.engine.data.jobchain.JobChainPath
import com.sos.scheduler.engine.data.scheduler.ClusterMemberId
import com.sos.scheduler.engine.kernel.persistence.hibernate.RichEntityManager.toRichEntityManager
import com.sos.scheduler.engine.persistence.entities.{JobChainEntity, JobChainNodeEntity}
import com.sos.scheduler.engine.test.configuration.TestConfiguration
import com.sos.scheduler.engine.test.{ProvidesTestEnvironment, TestSchedulerController}
import com.sos.scheduler.engine.tests.jira.js1151.JS1151IT._
import javax.persistence.EntityManagerFactory
import org.junit.runner.RunWith
import org.scalatest.FreeSpec
import org.scalatest.Matchers._
import org.scalatest.junit.JUnitRunner

@RunWith(classOf[JUnitRunner])
final class JS1151IT extends FreeSpec {
  private val testConfiguration = TestConfiguration(testClass = getClass)

  "Non-distributed JobScheduler: Database column cluster_member_id should contain '-'" in {
    autoClosing(ProvidesTestEnvironment(testConfiguration)) { envProvider =>
      envProvider.runScheduler() { implicit controller =>
        checkJobChain(notDistributedJobChainPath, "-")
        checkJobChainNode(notDistributedJobChainPath, "-")
      }
    }
  }

  "Distributed JobScheduler: Database column cluster_member_id should contain '-'" in {
    autoClosing(ProvidesTestEnvironment(testConfiguration.copy(mainArguments = List("-distributed-orders")))) { envProvider =>
      envProvider.runScheduler() { implicit controller =>
        //Not supported, SCHEDULER-384: checkJobChain(distributedJobChainPath, "-")
        checkJobChainNode(distributedJobChainPath, "-")
        checkJobChain(notDistributedJobChainPath, controller.instance[ClusterMemberId].string)
        checkJobChainNode(notDistributedJobChainPath, controller.instance[ClusterMemberId].string)
      }
    }
  }

  private def checkJobChain(jobChainPath: JobChainPath, expectedClusterMemberId: String)(implicit controller: TestSchedulerController): Unit = {
    import controller._
    deleteTable("SCHEDULER_JOB_CHAINS")
    scheduler executeXml <job_chain.modify job_chain={jobChainPath.string} state="stopped"/>
    val entityManager = instance[EntityManagerFactory].createEntityManager()
    try {
      val jobChainEntities = entityManager.fetchSeq[JobChainEntity]("select t from JobChainEntity t")
      jobChainEntities map { o ⇒ (o.jobChainPath, o.clusterMemberId, o.isStopped)} shouldEqual List(
        (jobChainPath.withoutStartingSlash, expectedClusterMemberId, true))
    }
    finally entityManager.close()
  }

  private def checkJobChainNode(jobChainPath: JobChainPath, expectedClusterMemberId: String)(implicit controller: TestSchedulerController): Unit = {
    import controller._
    deleteTable("SCHEDULER_JOB_CHAIN_NODES")
    scheduler executeXml <job_chain_node.modify job_chain={jobChainPath.string} state="100" action="stop"/>
    val entityManager = instance[EntityManagerFactory].createEntityManager()
    try {
      val nodesEntities = entityManager.fetchSeq[JobChainNodeEntity]("select t from JobChainNodeEntity t order by t.jobChainPath, t.clusterMemberId")
      nodesEntities map { o ⇒ (o.jobChainPath, o.clusterMemberId, o.orderState, o.action) } shouldEqual List(
        (jobChainPath.withoutStartingSlash, expectedClusterMemberId, "100", "stop"))
    }
    finally entityManager.close()
  }

  private def deleteTable(name: String)(implicit controller: TestSchedulerController): Unit = {
    autoClosing(controller.newJDBCConnection()) { connection ⇒
      val statement = connection.createStatement()
      statement.execute(s"""delete from "$name"""")
      statement.execute("commit")
    }
  }

//  private def deleteTable(name: String)(implicit controller: TestSchedulerController) {
//    Gibt einen JPA-Fehler beim Beenden des Schedulers.
//    import controller._
//    autoClosing(instance[EntityManagerFactory]) { implicit entityManagerFactory ⇒
//      transaction { entityManager ⇒
//        entityManager.createQuery(s"delete from $name").executeUpdate()
//      }
//    }
//  }
}

private object JS1151IT {
  private val notDistributedJobChainPath = JobChainPath("/test")
  private val distributedJobChainPath = JobChainPath("/test-distributed")
}
