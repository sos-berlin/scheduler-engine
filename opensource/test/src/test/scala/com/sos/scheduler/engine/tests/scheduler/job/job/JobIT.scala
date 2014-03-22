package com.sos.scheduler.engine.tests.scheduler.job.job

import JobIT._
import com.sos.scheduler.engine.data.job.JobPath
import com.sos.scheduler.engine.kernel.job.JobSubsystem
import com.sos.scheduler.engine.test.scala.ScalaSchedulerTest
import org.junit.runner.RunWith
import org.scalatest.FunSuite
import org.scalatest.Matchers._
import org.scalatest.junit.JUnitRunner

@RunWith(classOf[JUnitRunner])
class JobIT extends FunSuite with ScalaSchedulerTest {

  private lazy val job = instance[JobSubsystem].job(jobPath)

  test("job.name") {
    assert(job.name === "a")
  }

  test("job.path") {
    assert(job.path.string === "/a")
  }

  test("job.fileBasedIsReread") {
    assert(job.fileBasedIsReread === false)
  }

  test("jobSubsystem.visibleNames") {
    instance[JobSubsystem].visibleNames.toSet shouldEqual Set("a", "b")
  }

  test("jobSubsystem.names") {
    instance[JobSubsystem].names.toSet shouldEqual Set("scheduler_file_order_sink", "scheduler_service_forwarder", "a", "b")
  }
}


private object JobIT {
  private val jobPath = JobPath("/a")
}
