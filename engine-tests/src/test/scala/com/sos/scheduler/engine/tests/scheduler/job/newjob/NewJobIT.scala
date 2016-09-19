package com.sos.scheduler.engine.tests.scheduler.job.newjob

import com.sos.scheduler.engine.data.job.JobPath
import com.sos.scheduler.engine.test.scalatest.ScalaSchedulerTest
import org.junit.runner.RunWith
import org.scalatest.FunSuite
import org.scalatest.junit.JUnitRunner

@RunWith(classOf[JUnitRunner])
final class NewJobIT extends FunSuite with ScalaSchedulerTest {

//  private lazy val job = instance[JobSubsystemClient].job(jobPath)
//  private lazy val jobOverview = instance[JobSubsystemClient].jobOverview(jobPath)
//  private val blockingQueue = new ArrayBlockingQueue[Boolean](1)
//
//  if (false) {
//    test("job.name") {
//      assert(jobOverview.path.name === "test-a")
//    }
//
//    test("job.path") {
//      assert(jobOverview.path.string === "/test-a")
//    }
//
//    test("job.isFileBasedReread") {
//      assert(job.fileBasedIsReread === false)
//    }
//
//    test("jobSubsystem.visiblePaths") {
//      instance[JobSubsystemClient].visiblePaths.toSet shouldEqual Set(JobPath("/test-a"))
//    }
//
//    test("jobSubsystem.names") {
//      instance[JobSubsystemClient].paths.toSet shouldEqual Set(JobPath("/scheduler_file_order_sink"), JobPath("/scheduler_service_forwarder"), JobPath("/test-a"))
//    }
//
//    test("start") {
//      scheduler executeXml <start_job job={jobPath.string}/>
//      val ok = blockingQueue.poll(5, TimeUnit.SECONDS)
//      ok should be (true)
//    }
//
//  @EventHandler def handle(e: TaskEnded): Unit = {
//    if (e.jobPath == jobPath)
//      blockingQueue.offer(true)
//  }
}

object NewJobIT {
  private val jobPath = JobPath("/test-a")
}
