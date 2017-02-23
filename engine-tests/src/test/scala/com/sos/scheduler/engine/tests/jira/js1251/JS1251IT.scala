package com.sos.scheduler.engine.tests.jira.js1251

import com.sos.jobscheduler.common.scalautil.FileUtils.implicits._
import com.sos.scheduler.engine.data.filebased.FileBasedActivated
import com.sos.scheduler.engine.data.jobchain.{JobChainPath, NodeId}
import com.sos.scheduler.engine.data.order.{OrderFinished, OrderKey, OrderStepEnded}
import com.sos.scheduler.engine.data.xmlcommands.ModifyOrderCommand
import com.sos.scheduler.engine.kernel.folder.FolderSubsystemClient
import com.sos.scheduler.engine.kernel.persistence.hibernate.HibernateOrderStore
import com.sos.scheduler.engine.kernel.persistence.hibernate.ScalaHibernate.transaction
import com.sos.scheduler.engine.test.EventBusTestFutures.implicits._
import com.sos.scheduler.engine.test.configuration.TestConfiguration
import com.sos.scheduler.engine.test.scalatest.ScalaSchedulerTest
import com.sos.scheduler.engine.tests.jira.js1251.JS1251IT._
import javax.persistence.EntityManagerFactory
import org.junit.runner.RunWith
import org.scalatest.FreeSpec
import org.scalatest.Matchers._
import org.scalatest.junit.JUnitRunner

/**
 * @author Joacim Zschimmer
 */
@RunWith(classOf[JUnitRunner])
final class JS1251IT extends FreeSpec with ScalaSchedulerTest {

  override protected lazy val testConfiguration = TestConfiguration(getClass,
    mainArguments = List("-distributed-orders"))

  private implicit lazy val entityManager = instance[EntityManagerFactory]
  private implicit lazy val orderStore = instance[HibernateOrderStore]

  "Permanent order in a distributed job chain" in {
    eventBus.awaiting[OrderFinished](TestOrderKey) {
      scheduler executeXml ModifyOrderCommand(TestOrderKey, at = Some(ModifyOrderCommand.NowAt))
    }
    transaction { implicit entityManager ⇒
      orderStore.fetch(TestOrderKey).title shouldEqual OriginalTitle
    }
  }

  "Changing the order configuration file" in {
    file(TestOrderKey).contentString = file(TestOrderKey).contentString.replace(OriginalTitle, AChangedTitle)
    instance[FolderSubsystemClient].updateFolders()
    transaction { implicit entityManager ⇒
      orderStore.fetch(TestOrderKey).title shouldEqual AChangedTitle
    }
    eventBus.awaiting[OrderFinished](TestOrderKey) {
      scheduler executeXml ModifyOrderCommand(TestOrderKey, at = Some(ModifyOrderCommand.NowAt))
    }
  }

  "Changing the order configuration file while order is running takes effect when it is finished" in {
    scheduler executeXml <job_chain_node.modify job_chain={TestOrderKey.jobChainPath.string} state={SuspendedNodeId.string} action="stop"/>
    eventBus.awaiting[OrderStepEnded](TestOrderKey) {
      scheduler executeXml ModifyOrderCommand(TestOrderKey, at = Some(ModifyOrderCommand.NowAt))
    }
    eventBus.awaiting[FileBasedActivated.type](TestOrderKey) {
      file(TestOrderKey).contentString = file(TestOrderKey).contentString.replace(AChangedTitle, BChangedTitle)
      instance[FolderSubsystemClient].updateFolders()
      transaction { implicit entityManager ⇒
        orderStore.fetch(TestOrderKey) should have ('nodeIdOption(Some(SuspendedNodeId)), 'title(AChangedTitle))
      }
      eventBus.awaiting[OrderFinished](TestOrderKey) {
        scheduler executeXml <job_chain_node.modify job_chain={TestOrderKey.jobChainPath.string} state={SuspendedNodeId.string} action="process"/>
      }
    }
    transaction { implicit entityManager ⇒
      orderStore.fetch(TestOrderKey) should have ('nodeIdOption(Some(FirstNodeId)), 'title(BChangedTitle))
    }
  }

  "JS-1298 Non-distributed job chain in a distributed JobScheduler" in {
    eventBus.awaiting[OrderFinished](NonDistributedOrderKey) {
      scheduler executeXml ModifyOrderCommand(NonDistributedOrderKey, at = Some(ModifyOrderCommand.NowAt))
    }
  }

  private def file(o: OrderKey) = testEnvironment fileFromPath o
}

private object JS1251IT {
  private val TestJobChainPath = JobChainPath("/test")
  private val TestOrderKey = TestJobChainPath orderKey "1"
  private val OriginalTitle = "ORIGINAL TITLE"
  private val AChangedTitle = "CHANGED-A"
  private val BChangedTitle = "CHANGED-B"
  private val FirstNodeId = NodeId("100")
  private val SuspendedNodeId = NodeId("200")
  private val NonDistributedJobChainPath = JobChainPath("/test-non-distributed")
  private val NonDistributedOrderKey = JobChainPath("/test-non-distributed") orderKey "1"
}
