package com.sos.scheduler.engine.tests.scheduler.job.job

import scala.collection.JavaConversions._
import org.scalatest.matchers.ShouldMatchers._
import com.sos.scheduler.engine.data.folder.AbsolutePath
import com.sos.scheduler.engine.test.scala.ScalaSchedulerTest
import org.junit.runner.RunWith
import org.scalatest.junit.JUnitRunner

@RunWith(classOf[JUnitRunner])
class JobTest extends ScalaSchedulerTest {
  import JobTest._

  private lazy val job = scheduler.getJobSubsystem.job(jobPath)

  test("job.name") {
    assert(job.getName === "a")
  }

  test("job.path") {
    assert(job.getPath.toString === "/a")
  }

  test("job.isFileBasedReread") {
    assert(job.isFileBasedReread === false)
  }

  test("jobSubsystem.visibleNames") {
    val list = scheduler.getJobSubsystem.getVisibleNames.toList
    list.toSet should equal (Set("a", "b"))
  }

  test("jobSubsystem.names") {
    val list = scheduler.getJobSubsystem.getNames.toList
    list.toSet should equal (Set("scheduler_file_order_sink", "scheduler_service_forwarder", "a", "b"))
  }
}

object JobTest {
  private val jobPath = new AbsolutePath("/a")
}
