package com.sos.scheduler.engine.tests.jira.js1300

import com.google.common.io.Files.touch
import com.sos.scheduler.engine.agent.test.AgentTest
import com.sos.scheduler.engine.common.scalautil.FileUtils.implicits._
import com.sos.scheduler.engine.common.time.ScalaTime._
import com.sos.scheduler.engine.data.job.JobPath
import com.sos.scheduler.engine.data.jobchain.JobChainPath
import com.sos.scheduler.engine.data.log.InfoLogEvent
import com.sos.scheduler.engine.data.message.MessageCode
import com.sos.scheduler.engine.data.order.{OrderFinishedEvent, OrderKey, OrderTouchedEvent}
import com.sos.scheduler.engine.kernel.persistence.hibernate.HibernateOrderStore
import com.sos.scheduler.engine.kernel.persistence.hibernate.ScalaHibernate._
import com.sos.scheduler.engine.test.EventBusTestFutures.implicits.RichEventBus
import com.sos.scheduler.engine.test.SchedulerTestUtils._
import com.sos.scheduler.engine.test.configuration.TestConfiguration
import com.sos.scheduler.engine.test.scalatest.ScalaSchedulerTest
import com.sos.scheduler.engine.tests.jira.js1300.JS1300IT._
import java.nio.file.Files.delete
import java.nio.file.Path
import java.time.Duration
import javax.persistence.EntityManagerFactory
import org.junit.runner.RunWith
import org.scalatest.FreeSpec
import org.scalatest.junit.JUnitRunner

/**
 * JS-1300 <file_order_source remote_scheduler="..">
 *
 * @author Joacim Zschimmer
 */
@RunWith(classOf[JUnitRunner])
final class JS1300IT extends FreeSpec with ScalaSchedulerTest with AgentTest {

  override protected lazy val testConfiguration = TestConfiguration(getClass, mainArguments = List("-distributed-orders"))
  private implicit lazy val entityManagerFactory = instance[EntityManagerFactory]
  private lazy val directory = testEnvironment.newFileOrderSourceDirectory()
  private lazy val matchingFile = directory / "X-MATCHING-FILE"

  for (isDistributed ← List(false, true)) {
    (if (isDistributed) "Distributed" else "Not distributed") - {
      "Some files, one after the other" in {
        val repeat = if (isDistributed) 1.s else 3600.s  // Short period when distributed, because JobScheduler cannot immediately check file existence via Agent (because the order vanishes)
        scheduler executeXml newJobChainElem(directory, agentUri = agentUri, JobPath("/test-delete"), repeat = repeat, isDistributed = isDistributed)
        for (_ ← 1 to 3) {
          val orderKey = TestJobChainPath orderKey matchingFile.toString
          eventBus.awaitingEvent[InfoLogEvent](_.codeOption contains MessageCode("SCHEDULER-981")) {
            eventBus.awaitingKeyedEvent[OrderFinishedEvent](orderKey) {
              touch(matchingFile)
            }
          }
        }
      }

      "A file, not deleted by job chain, stays on blacklist" in {
        val repeat = 1.s
        val delay = repeat dividedBy 2
        scheduler executeXml newJobChainElem(directory, agentUri = agentUri, JobPath("/test-dont-delete"), repeat, isDistributed = isDistributed)
        val file = directory / "MATCHING-TEST-DONT-DELETE"
        val orderKey = TestJobChainPath orderKey file.toString
        eventBus.awaitingEvent[InfoLogEvent](_.codeOption contains MessageCode("SCHEDULER-981")) {
          eventBus.awaitingKeyedEvent[OrderFinishedEvent](orderKey) {
            touch(file)
          }
          val startedAgain = eventBus.keyedEventFuture[OrderTouchedEvent](orderKey)
          assert(orderIsOnBlacklist(orderKey))
          sleep(repeat + delay)
          assert(orderIsOnBlacklist(orderKey))
          assert(!startedAgain.isCompleted)
          delete(file)
        }
        assert(!orderExists(orderKey))
        eventBus.awaitingEvent[InfoLogEvent](_.codeOption contains MessageCode("SCHEDULER-981")) {
          eventBus.awaitingKeyedEvent[OrderFinishedEvent](orderKey) {
            val started = eventBus.keyedEventFuture[OrderTouchedEvent](orderKey)
            assert(!started.isCompleted)
            sleep(repeat + delay)
            assert(!started.isCompleted)
            touch(file)
          }
          assert(orderIsOnBlacklist(orderKey))
          delete(file)
        }
      }
    }
  }

  "regex filters files" in {
    scheduler executeXml newJobChainElem(directory, agentUri = agentUri, JobPath("/test-delete"), repeat = 3600.s, isDistributed = false)
    val ignoredFile = directory / "IGNORED-FILE"
    val List(matchingOrderKey, ignoredOrderKey) = for (file ← List(matchingFile, ignoredFile)) yield {
      touch(file)
      TestJobChainPath orderKey file.toString
    }
    val ignoredStarted = eventBus.keyedEventFuture[OrderTouchedEvent](ignoredOrderKey)
    eventBus.awaitingEvent[InfoLogEvent](_.codeOption contains MessageCode("SCHEDULER-981")) {
      eventBus.awaitingKeyedEvent[OrderFinishedEvent](matchingOrderKey) {
        touch(matchingFile)
      }
    }
    sleep(2.s)
    assert(!ignoredStarted.isCompleted)
  }

  private def orderIsOnBlacklist(orderKey: OrderKey): Boolean =
    if (jobChain(orderKey.jobChainPath).isDistributed)
      transaction { implicit entityManager ⇒
        val e = instance[HibernateOrderStore].fetch(orderKey)
        e.isOnBlacklist
      }
    else
      order(orderKey).isOnBlacklist
}

private object JS1300IT {
  private val TestJobChainPath = JobChainPath("/test")

  private def newJobChainElem(directory: Path, agentUri: String, jobPath: JobPath, repeat: Duration, isDistributed: Boolean): xml.Elem =
    <job_chain name={TestJobChainPath.withoutStartingSlash} distributed={isDistributed.toString}>
      <file_order_source directory={directory.toString} regex="MATCHING-" remote_scheduler={agentUri} repeat={repeat.getSeconds.toString}/>
      <job_chain_node state="100" job={jobPath.string}/>
      <job_chain_node.end state="END"/>
    </job_chain>
}
