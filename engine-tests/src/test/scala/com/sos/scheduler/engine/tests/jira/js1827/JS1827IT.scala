package com.sos.scheduler.engine.tests.jira.js1827

import com.sos.scheduler.engine.base.sprayjson.SprayJson._
import com.sos.scheduler.engine.client.web.StandardWebSchedulerClient
import com.sos.scheduler.engine.common.scalautil.Closers.implicits.RichClosersAutoCloseable
import com.sos.scheduler.engine.common.scalautil.FileUtils.deleteDirectoryRecursively
import com.sos.scheduler.engine.common.scalautil.FileUtils.implicits._
import com.sos.scheduler.engine.common.scalautil.Futures.implicits._
import com.sos.scheduler.engine.common.sprayutils.JsObjectMarshallers._
import com.sos.scheduler.engine.common.time.ScalaTime._
import com.sos.scheduler.engine.common.utils.FreeTcpPortFinder.findRandomFreeTcpPort
import com.sos.scheduler.engine.data.filebased.FileBasedRemoved
import com.sos.scheduler.engine.data.folder.FolderPath
import com.sos.scheduler.engine.data.job.JobPath
import com.sos.scheduler.engine.data.order.JocOrderStatistics
import com.sos.scheduler.engine.kernel.folder.FolderSubsystemClient
import com.sos.scheduler.engine.test.EventBusTestFutures.implicits.RichEventBus
import com.sos.scheduler.engine.test.configuration.TestConfiguration
import com.sos.scheduler.engine.test.scalatest.ScalaSchedulerTest
import org.junit.runner.RunWith
import org.scalatest.FreeSpec
import org.scalatest.junit.JUnitRunner
import spray.json._

/**
  * @author Joacim Zschimmer
  */
@RunWith(classOf[JUnitRunner])
final class JS1827IT extends FreeSpec with ScalaSchedulerTest
{
  private lazy val httpPort = findRandomFreeTcpPort()
  protected lazy val client = new StandardWebSchedulerClient(s"http://127.0.0.1:$httpPort").closeWithCloser
  protected override lazy val testConfiguration = TestConfiguration(getClass,
    mainArguments = List(s"-http-port=127.0.0.1:$httpPort"))

  "Web service JocOrderStatistics with a folder" in {
    assert(fetchJocOrderStatistics() == Map(
      JobPath("/JS-1827/test") -> JocOrderStatistics.Zero,
      JobPath("/scheduler_file_order_sink") -> JocOrderStatistics.Zero,
      JobPath("/scheduler_service_forwarder") -> JocOrderStatistics.Zero))
  }

  "Web service JocOrderStatistics after the folder has been deleted" in {
    controller.eventBus.awaiting[FileBasedRemoved.type](FolderPath("/JS-1827")) {
      deleteDirectoryRecursively(testEnvironment.liveDirectory / "JS-1827")
      instance[FolderSubsystemClient].updateFolders()
    }
    assert(fetchJocOrderStatistics() == Map(
      JobPath("/scheduler_file_order_sink") -> JocOrderStatistics.Zero,
      JobPath("/scheduler_service_forwarder") -> JocOrderStatistics.Zero))
  }

  private def fetchJocOrderStatistics(): Map[JobPath, JocOrderStatistics] =
    client.postByUri[JsObject, JsObject](
      relativeUri = "api/job?return=JocOrderStatistics&isDistributed=false",
      data = json"""{ "path": "/" }""".asJsObject
    ).await(99.s)
    .fields map { case (k, v) => JobPath(k) -> v.convertTo[JocOrderStatistics] }
}
