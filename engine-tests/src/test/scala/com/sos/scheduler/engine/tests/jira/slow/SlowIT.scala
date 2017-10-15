package com.sos.scheduler.engine.tests.jira.slow

import com.google.common.io.Closer
import com.google.common.io.Files.touch
import com.sos.scheduler.engine.common.scalautil.FileUtils.implicits._
import com.sos.scheduler.engine.common.scalautil.Futures.implicits._
import com.sos.scheduler.engine.common.scalautil.Logger
import com.sos.scheduler.engine.common.time.ScalaTime._
import com.sos.scheduler.engine.data.job.{JobPath, TaskStartedEvent}
import com.sos.scheduler.engine.data.jobchain.JobChainPath
import com.sos.scheduler.engine.data.order.OrderFinishedEvent
import com.sos.scheduler.engine.test.SchedulerTestUtils.writeConfigurationFile
import com.sos.scheduler.engine.test.binary.CppBinariesDebugMode
import com.sos.scheduler.engine.test.configuration.TestConfiguration
import com.sos.scheduler.engine.test.scalatest.ScalaSchedulerTest
import com.sos.scheduler.engine.tests.jira.slow.SlowIT._
import java.nio.file.Files.createDirectory
import java.nio.file.{Path, Paths}
import java.time.Duration
import java.time.Instant.now
import org.junit.runner.RunWith
import org.scalatest.FreeSpec
import org.scalatest.junit.JUnitRunner
import scala.collection.mutable
import scala.concurrent.{Future, Promise}

/**
  * @author Joacim Zschimmer
  */
@RunWith(classOf[JUnitRunner])
final class SlowIT extends FreeSpec with ScalaSchedulerTest {

  private val jobChainCount = sys.props.get("test.jobChains") map (_.toInt) getOrElse 18
  private val jobCount = sys.props.get("test.jobs") map (_.toInt) getOrElse 100
  private val orderCount = sys.props.get("test.orders") map (_.toInt) getOrElse 12

  override protected lazy val testConfiguration = TestConfiguration(getClass,
    binariesDebugMode = Some(CppBinariesDebugMode.release))

  if (sys.props contains "test.speed") "Speed test" in {
    val root = testEnvironment.newFileOrderSourceDirectory()
    val dir = root / "dir"
    createDirectory(dir)
    writeConfigurationFile(JobChainPath("/test"),
      <job_chain>
        <file_order_source directory={dir.getPath}/>
        <job_chain_node state="100" job="/quick"/>
        <job_chain_node state="200" job="/slow"/>
        <file_order_sink state="SINK" remove="true"/>
      </job_chain>)
    runFileOrders(dir)

    createIdleJobChains(root, jobChainCount)
    runFileOrders(dir)

    createOtherJobs(jobCount)
    runFileOrders(dir)

    val permanentOrderGenerator = new PermanentOrderGenerator(orderCount)
    permanentOrderGenerator.createJobChains()
    runFileOrders(dir)

    permanentOrderGenerator.createOrders()
    runFileOrders(dir)
  }

  private class PermanentOrderGenerator(n: Int) {
    private val jobChainPaths = for (i ← 1 to n) yield JobChainPath(s"/permanent-$i")
    private val jobPaths = for (i ← 1 to n) yield JobPath(s"/permanent-$i")

    def createJobChains(): Unit = {
      val jobsStarted = whenJobsStarted(jobPaths)
      for ((jobChainPath, jobPath) ← jobChainPaths zip jobPaths) {
        writeConfigurationFile(jobPath,
          <job order="true" min_tasks="1">
            <script java_class="com.sos.scheduler.engine.tests.jira.slow.QuickJob"/>
          </job>)
        writeConfigurationFile(jobChainPath,
          <job_chain>
            <job_chain_node state="100" job={jobPath.string}/>
            <job_chain_node.end state="END"/>
          </job_chain>)
      }
      jobsStarted await 99.s
    }

    def createOrders(): Unit = {
      for (jobChainPath ← jobChainPaths) {
        writeConfigurationFile(jobChainPath orderKey "1",
          <order>
            <run_time/>
          </order>)
      }
    }
  }

  private def createIdleJobChains(rootDir: Path, n: Int): Unit = {
    val jobPaths = for (i ← 1 to n) yield JobPath(s"/idle-$i")
    val jobsStarted = whenJobsStarted(jobPaths)
    for ((i, jobPath) ← 1 to n zip jobPaths) {
      val dir = rootDir / s"$i"
      createDirectory(dir)
      writeConfigurationFile(jobPath,
        <job order="true" min_tasks="1">
          <script java_class="com.sos.scheduler.engine.tests.jira.slow.QuickJob"/>
        </job>)
      writeConfigurationFile(JobChainPath(s"/idle-$i"),
        <job_chain>
          <file_order_source directory={dir.getPath}/>
          <job_chain_node state="100" job={jobPath.string}/>
          <file_order_sink state="SINK" remove="true"/>
        </job_chain>)
    }
    jobsStarted await 99.s
  }

  private def whenJobsStarted(jobPaths: Iterable[JobPath]): Future[Unit] = {
    implicit val closer = Closer.create()
    val promise = Promise[Unit]()
    val remaining = jobPaths.to[mutable.Set]
    eventBus.on[TaskStartedEvent] { case e ⇒
      remaining -= e.jobPath
      if (remaining.isEmpty) {
        promise.success(())
        closer.close()
      }
    }
    promise.future
  }

  private def createOtherJobs(n: Int): Unit =
    for (i ← 1 to n) {
      writeConfigurationFile(JobPath(s"/other-$i"),
        <job>
          <script java_class="com.sos.scheduler.engine.tests.jira.slow.QuickJob"/>
        </job>)
    }

  private def runFileOrders(dir: Path) = {
    val files = for (i ← 1 to 3) yield dir / s"$i"
    val whenDurations = waitForOrders(files.size)
    for (f ← files) touch(f)
    val durations = whenDurations await 99.s
    logger.info("####### " + (durations map (_.pretty) mkString " "))
    scheduler executeXml <modify_job job="/quick" cmd="end"/>
    scheduler executeXml <modify_job job="/slow" cmd="end"/>
  }

  private def waitForOrders(n: Int): Future[Vector[Duration]] = {
    val startTime = now
    val finished = mutable.Map[Int, Duration]()
    val promise = Promise[Vector[Duration]]()
    implicit val closer = Closer.create()
    eventBus.on[OrderFinishedEvent] { case e ⇒
      val filename = Paths.get(e.orderKey.id.string).getFileName.getPath
      val i = filename.toInt
      finished(i) = now - startTime
      if (finished.size == n) {
        closer.close()  // Unsubscript
        promise.success(finished.toVector.sortBy(_._1).map(_._2))
      }
    }
    promise.future
  }
}

object SlowIT {
  private val logger = Logger(getClass)
}
