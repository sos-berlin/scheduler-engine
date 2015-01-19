package com.sos.scheduler.engine.tests.jira.js1251

import com.sos.scheduler.engine.common.scalautil.FileUtils.implicits._
import com.sos.scheduler.engine.data.filebased.FileBasedActivatedEvent
import com.sos.scheduler.engine.data.jobchain.JobChainPath
import com.sos.scheduler.engine.data.order.{OrderFinishedEvent, OrderKey, OrderState, OrderStepEndedEvent}
import com.sos.scheduler.engine.data.xmlcommands.ModifyOrderCommand
import com.sos.scheduler.engine.kernel.folder.FolderSubsystem
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

  import controller.eventBus

  override protected lazy val testConfiguration = TestConfiguration(getClass,
    mainArguments = List("-distributed-orders"))

  private implicit lazy val entityManager = instance[EntityManagerFactory]
  private implicit lazy val orderStore = instance[HibernateOrderStore]

  "Permanent order in a distributed job chain" in {
    eventBus.awaitingKeyedEvent[OrderFinishedEvent](TestOrderKey) {
      scheduler executeXml ModifyOrderCommand(TestOrderKey, at = Some(ModifyOrderCommand.NowAt))
    }
    transaction { implicit entityManager ⇒
      orderStore.fetch(TestOrderKey).title shouldEqual OriginalTitle
    }
  }

  "Changing the order configuration file" in {
    file(TestOrderKey).contentString = file(TestOrderKey).contentString.replace(OriginalTitle, AChangedTitle)
    instance[FolderSubsystem].updateFolders()
    transaction { implicit entityManager ⇒
      orderStore.fetch(TestOrderKey).title shouldEqual AChangedTitle
    }
    eventBus.awaitingKeyedEvent[OrderFinishedEvent](TestOrderKey) {
      scheduler executeXml ModifyOrderCommand(TestOrderKey, at = Some(ModifyOrderCommand.NowAt))
    }
  }

  "Changing the order configuration file while order is running takes effect when it is finished" in {
    scheduler executeXml <job_chain_node.modify job_chain={TestOrderKey.jobChainPath.string} state={SuspendedState.string} action="stop"/>
    eventBus.awaitingKeyedEvent[OrderStepEndedEvent](TestOrderKey) {
      scheduler executeXml ModifyOrderCommand(TestOrderKey, at = Some(ModifyOrderCommand.NowAt))
    }
    eventBus.awaitingKeyedEvent[FileBasedActivatedEvent](TestOrderKey) {
      file(TestOrderKey).contentString = file(TestOrderKey).contentString.replace(AChangedTitle, BChangedTitle)
      instance[FolderSubsystem].updateFolders()
      transaction { implicit entityManager ⇒
        orderStore.fetch(TestOrderKey) should have ('stateOption(Some(SuspendedState)), 'title(AChangedTitle))
      }
      eventBus.awaitingKeyedEvent[OrderFinishedEvent](TestOrderKey) {
        scheduler executeXml <job_chain_node.modify job_chain={TestOrderKey.jobChainPath.string} state={SuspendedState.string} action="process"/>
      }
    }
    transaction { implicit entityManager ⇒
      orderStore.fetch(TestOrderKey) should have ('stateOption(Some(FirstState)), 'title(BChangedTitle))
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
  private val FirstState = OrderState("100")
  private val SuspendedState = OrderState("200")
}
