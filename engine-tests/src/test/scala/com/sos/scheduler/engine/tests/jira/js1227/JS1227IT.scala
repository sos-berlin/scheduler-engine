package com.sos.scheduler.engine.tests.jira.js1227

import com.sos.jobscheduler.common.scalautil.xmls.SafeXML
import com.sos.scheduler.engine.data.job.JobPath
import com.sos.scheduler.engine.data.jobchain.{JobChainPath, NodeId}
import com.sos.jobscheduler.data.message.MessageCode
import com.sos.scheduler.engine.data.order.{OrderStarted, OrderStepEnded, OrderSuspended}
import com.sos.scheduler.engine.data.xmlcommands.{ModifyJobCommand, ModifyOrderCommand, OrderCommand}
import com.sos.scheduler.engine.kernel.persistence.hibernate.HibernateOrderStore
import com.sos.scheduler.engine.kernel.persistence.hibernate.ScalaHibernate._
import com.sos.scheduler.engine.test.ClusterTest
import com.sos.scheduler.engine.test.EventBusTestFutures.implicits._
import com.sos.scheduler.engine.test.SchedulerTestUtils.{awaitFailure, awaitSuccess}
import com.sos.scheduler.engine.tests.jira.js1227.JS1227IT._
import javax.persistence.EntityManagerFactory
import org.junit.runner.RunWith
import org.scalatest.FreeSpec
import org.scalatest.junit.JUnitRunner

/**
 * JS-1227 An order executed on any cluster member should be suspendable.
 * @author Joacim Zschimmer
 */
@RunWith(classOf[JUnitRunner])
final class JS1227IT extends FreeSpec with ClusterTest {

  protected def clusterMemberCount = 2
  private implicit lazy val entityManagerFactory = instance[EntityManagerFactory]

  private lazy val otherScheduler = otherSchedulers(0)

  "Suspend order running in some other scheduler" in {
    awaitSuccess(otherScheduler.postCommand(ModifyJobCommand(TestJobPath, cmd = Some(ModifyJobCommand.Cmd.Stop))))
    eventBus.awaiting[OrderStarted.type](AOrderKey) {
      scheduler executeXml OrderCommand(AOrderKey)
    }
    eventBus.awaiting[OrderSuspended.type](AOrderKey) {
      awaitFailure(otherScheduler.postCommand(ModifyOrderCommand(AOrderKey, suspended = Some(true)))) match {
        case e: Exception if e.getMessage startsWith s"$OrderIsOccupiedMessageCode " ⇒
      }
      eventBus.awaiting[OrderStepEnded](AOrderKey) {}
      transaction { implicit entityManager ⇒
        val entity = instance[HibernateOrderStore].fetch(AOrderKey)
        assert(entity.nodeIdOption == Some(NodeId("200")))
        val e = SafeXML.loadString(entity.xmlOption.get)
        if (e \@ "suspended" != "yes") fail("Order should be suspended")
      }
    }
  }
}

private object JS1227IT {
  private val AOrderKey = JobChainPath("/test") orderKey "A"
  private val TestJobPath = JobPath("/test")
  private val OrderIsOccupiedMessageCode = MessageCode("SCHEDULER-379")
}
