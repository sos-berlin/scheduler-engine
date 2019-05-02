package com.sos.scheduler.engine.tests.jira.js1795

import com.google.common.io.Files.touch
import com.sos.scheduler.engine.client.web.StandardWebSchedulerClient
import com.sos.scheduler.engine.common.scalautil.Closers.implicits.RichClosersAutoCloseable
import com.sos.scheduler.engine.common.scalautil.FileUtils.deleteDirectoryRecursively
import com.sos.scheduler.engine.common.scalautil.FileUtils.implicits._
import com.sos.scheduler.engine.common.scalautil.Futures.implicits.SuccessFuture
import com.sos.scheduler.engine.common.time.ScalaTime._
import com.sos.scheduler.engine.common.utils.FreeTcpPortFinder.findRandomFreeTcpPort
import com.sos.scheduler.engine.data.filebased.FileBasedState
import com.sos.scheduler.engine.data.job.JobPath
import com.sos.scheduler.engine.data.jobchain.{EndNodeOverview, JobChainDetailed, JobChainObstacle, JobChainOverview, JobChainPath, JobChainState, NodeId, SimpleJobNodeOverview}
import com.sos.scheduler.engine.data.log.ErrorLogged
import com.sos.scheduler.engine.data.message.MessageCode
import com.sos.scheduler.engine.data.order.{OrderHistoryId, OrderObstacle, OrderOverview, OrderProcessingState, OrderSourceType}
import com.sos.scheduler.engine.data.queries.JobChainQuery
import com.sos.scheduler.engine.test.EventBusTestFutures.implicits.RichEventBus
import com.sos.scheduler.engine.test.configuration.TestConfiguration
import com.sos.scheduler.engine.test.scalatest.ScalaSchedulerTest
import java.nio.file.Files.createTempDirectory
import java.time.Instant
import org.junit.runner.RunWith
import org.scalatest.FreeSpec
import org.scalatest.junit.JUnitRunner
import scala.concurrent.ExecutionContext

/**
  * @author Joacim Zschimmer
  */
@RunWith(classOf[JUnitRunner])
final class JS1795IT extends FreeSpec with ScalaSchedulerTest
{
  private lazy val httpPort = findRandomFreeTcpPort()
  protected override lazy val testConfiguration = TestConfiguration(getClass,
    mainArguments = List(s"-http-port=127.0.0.1:$httpPort"),
    ignoreError = Set(MessageCode("SCHEDULER-340")))
  private lazy val webSchedulerClient = new StandardWebSchedulerClient(s"http://127.0.0.1:$httpPort").closeWithCloser
  private implicit lazy val executionContext = instance[ExecutionContext]

  "Webservice returns blacklisted files" in {
    val directory = createTempDirectory("JS-1795-")
    val jobChainPath = JobChainPath("/FILES")
    val file1 = directory / "TEST-1"
    val file2 = directory / "TEST-2"
    scheduler executeXml
      <job_chain name={jobChainPath.name} max_orders="1">
        <file_order_source directory={directory.toString} repeat="1" delay_after_error="999"/>
        <job_chain_node state="100" job="/test"/>
        <job_chain_node.end state="END"/>
      </job_chain>
    eventBus.awaitingWhen[ErrorLogged](_.event.codeOption contains MessageCode("SCHEDULER-340")) {  // "blacklisted"
      touch(file1)
    }
    scheduler executeXml <job_chain.modify job_chain={jobChainPath.name} state="stopped"/>
    touch(file2)
    sleep(4.s)  // Wait until JobScheduler detects the file: 1s repeat interval + 2s check for unchanged file + 1s reserve
    val jobChainDetailedSeq = webSchedulerClient.jobChainDetailedBy(JobChainQuery.All).await(99.s).value
    val jobChainDetailed = jobChainDetailedSeq.find(_.overview.path == jobChainPath)
      .getOrElse(sys.error("Missing JobChainDetailed"))
    assert(jobChainDetailed.copy(blacklistedOrders = jobChainDetailed.blacklistedOrders.map(_.copy(startedAt = None))) ==
      JobChainDetailed(
        JobChainOverview(
          jobChainPath,
          FileBasedState.active,
          JobChainState.stopped,
          jobOrJobChainNodeCount = 1,
          nonBlacklistedOrderCount = 0,
          blacklistedOrderCount = 1,
          orderLimit = Some(1),
          obstacles = Set(JobChainObstacle.Stopped, JobChainObstacle.OrderLimitReached(1))),
        Vector(
          SimpleJobNodeOverview(
            jobChainPath,
            NodeId("100"),
            nextNodeId = NodeId("END"),
            errorNodeId = NodeId(""),
            JobPath("/test"),
            orderCount = 0),
          EndNodeOverview(
            jobChainPath,
            NodeId("END"))),
        Vector(
          JobChainDetailed.FileOrderSource(
            directory, regex = "", repeat = 1.s, delayAfterError = 999.s, alertWhenDirectoryMissing = true,
            Vector(
              JobChainDetailed.FileOrderSourceFile(file1, Instant.ofEpochSecond(file1.lastModified / 1000)),
              JobChainDetailed.FileOrderSourceFile(file2, Instant.ofEpochSecond(file2.lastModified / 1000))))),
        blacklistedOrders = Vector(
          OrderOverview(
            jobChainPath orderKey file1.toString,
            FileBasedState.not_initialized, OrderSourceType.FileOrder,
            jobChainPath, NodeId("END"), OrderProcessingState.Blacklisted, Some(OrderHistoryId(2)),
            Set(OrderObstacle.Blacklisted),
            None,  // Changed to None because of unpredictable value
            nextStepAt = Some(Instant.ofEpochMilli(0))))))
    deleteDirectoryRecursively(directory)
  }
}
