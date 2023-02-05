package com.sos.scheduler.engine.tests.jira.js2037

import com.sos.scheduler.engine.common.time.ScalaTime.DurationRichInt
import com.sos.scheduler.engine.common.time.WaitForCondition.waitForCondition
import com.sos.scheduler.engine.data.jobchain.JobChainPath
import com.sos.scheduler.engine.data.order.OrderKey
import com.sos.scheduler.engine.kernel.persistence.hibernate.HibernateOrderStore
import com.sos.scheduler.engine.kernel.persistence.hibernate.ScalaHibernate.transaction
import com.sos.scheduler.engine.test.{ClusterTest, TestSchedulerController}
import com.sos.scheduler.engine.tests.jira.js2037.JS2037IT._
import java.time.{Duration, Instant}
import javax.persistence.EntityManagerFactory
import org.junit.runner.RunWith
import org.scalatest.FreeSpec
import org.scalatest.junit.JUnitRunner

/**
 * JS-2037 Distributed Orders with a start time not below 2023-01-23 20:13:54Z are not started
 */
@RunWith(classOf[JUnitRunner])
final class JS2037IT extends FreeSpec with ClusterTest {

  override protected lazy val testConfiguration = super.testConfiguration.copy(
    logCategories = "scheduler scheduler.mainlog jdbc all")

  protected def clusterMemberCount = 1

  private implicit lazy val entityManagerFactory = instance[EntityManagerFactory]

  "Suspend order running in some other scheduler" in {
    scheduler executeXml toOrder(AOrderKey, at = Instant.now.plusSeconds(10).toString)
    scheduler executeXml toOrder(BOrderKey, at = Instant.now.plusSeconds(10).toString)

    waitForCondition(99.s, 100.ms) {
      !orderExists(AOrderKey) && !orderExists(BOrderKey)
    }
    assert(!orderExists(AOrderKey) && !orderExists(BOrderKey))
  }

  private def toOrder(orderKey: OrderKey, at: String) =
    <order
      job_chain={orderKey.jobChainPath.string}
      id={orderKey.id.string}
      at={at}
    />

  private def orderExists(orderKey: OrderKey) =
    transaction(entityManagerFactory) { implicit entityManager =>
      orderStore.tryFetch(orderKey).isDefined
    }

  private def orderStore(implicit controller: TestSchedulerController) =
    controller.instance[HibernateOrderStore]
}

private object JS2037IT {
  private val jobChainPath = JobChainPath("/JOB-CHAIN")
  private val AOrderKey = jobChainPath.orderKey("A-ORDER")
  private val BOrderKey = jobChainPath.orderKey("B-ORDER")
}
