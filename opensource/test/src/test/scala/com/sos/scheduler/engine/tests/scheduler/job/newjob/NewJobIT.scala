package com.sos.scheduler.engine.tests.scheduler.job.newjob

import NewJobIT._
import com.sos.scheduler.engine.data.job.{JobPath, TaskEndedEvent}
import com.sos.scheduler.engine.eventbus.EventHandler
import com.sos.scheduler.engine.kernel.job.JobSubsystem
import com.sos.scheduler.engine.test.scala.ScalaSchedulerTest
import com.sos.scheduler.engine.test.scala.SchedulerTestImplicits._
import java.util.concurrent.{TimeUnit, ArrayBlockingQueue}
import org.junit.runner.RunWith
import org.scalatest.FunSuite
import org.scalatest.Matchers._
import org.scalatest.junit.JUnitRunner
import scala.collection.JavaConversions._

@RunWith(classOf[JUnitRunner])
final class NewJobIT extends FunSuite with ScalaSchedulerTest {

  private lazy val job = instance[JobSubsystem].job(jobPath)
  private val blockingQueue = new ArrayBlockingQueue[Boolean](1)

  test("job.name") {
    assert(job.name === "test-a")
  }

  test("job.path") {
    assert(job.path.string === "/test-a")
  }

  test("job.isFileBasedReread") {
    assert(job.fileBasedIsReread === false)
  }

  test("jobSubsystem.visibleNames") {
    instance[JobSubsystem].visibleNames.toSet should equal (Set("test-a"))
  }

  test("jobSubsystem.names") {
    instance[JobSubsystem].names.toSet should equal (Set("scheduler_file_order_sink", "scheduler_service_forwarder", "test-a"))
  }

  test("start") {
    scheduler executeXml <start_job job={jobPath.string}/>
    val ok = blockingQueue.poll(5, TimeUnit.SECONDS)
    ok should be (true)
  }

  @EventHandler def handle(e: TaskEndedEvent) {
    if (e.jobPath == jobPath)
      blockingQueue.offer(true)
  }
}

object NewJobIT {
  private val jobPath = JobPath.of("/test-a")
}
