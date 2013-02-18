package com.sos.scheduler.engine.tests.scheduler.job.manyjobs

import ManyJobsIT._
import com.sos.scheduler.engine.common.time.ScalaJoda.{DurationRichInt, sleep}
import com.sos.scheduler.engine.data.folder.{FileBasedActivatedEvent, JobPath}
import com.sos.scheduler.engine.data.job.TaskStartedEvent
import com.sos.scheduler.engine.eventbus.EventHandler
import com.sos.scheduler.engine.test.scala.ScalaSchedulerTest
import com.sos.scheduler.engine.test.scala.SchedulerTestImplicits._
import com.sos.scheduler.engine.test.util.time.WaitForCondition.waitForCondition
import org.joda.time.Duration.standardSeconds
import org.junit.runner.RunWith
import org.scalatest.junit.JUnitRunner
import org.scalatest.matchers.ShouldMatchers._
import scala.collection.mutable

@RunWith(classOf[JUnitRunner])
class ManyJobsIT extends ScalaSchedulerTest {

  //override val binariesDebugMode = CppBinariesDebugMode.release
  private var activatedJobCount = 0
  private var taskCount = 0
  private lazy val jobs = 1 to n map JobDefinition
  private lazy val jobStatistics = mutable.HashMap[JobPath, JobStatistics]() ++ (jobs map { j => j.path -> JobStatistics() })

  ignore(s"Adding $n jobs") {
    for (j <- jobs) scheduler executeXml j.xmlElem
    waitForCondition(timeout=n*100.ms, step=100.ms) { activatedJobCount == n }
  }

  val duration = 4 * interval
  val expectedTaskCount = (duration.getStandardSeconds * tasksPerSecond).toInt

  ignore(s"$expectedTaskCount tasks should run in ${duration.getStandardSeconds}s ($tasksPerSecond tasks/s)") {
    scheduler executeXml <commands>{
      jobs map { o =>
          <modify_job job={o.path.string} cmd="enable"/>
          <modify_job job={o.path.string} cmd="unstop"/>
      }
    }</commands>
    sleep(duration)
    controller.getEventBus.dispatchEvents()
    taskCount should be (expectedTaskCount plusOrMinus (0.2 * expectedTaskCount).toInt)
    //(for (j <- jobStatistics) j.taskCount should equal (tasksPer)
  }

  @EventHandler def handle(e: FileBasedActivatedEvent) {
    e.getTypedPath collect { case p: JobPath if jobStatistics.keySet contains p => activatedJobCount += 1 }
  }

  @EventHandler def handle(e: TaskStartedEvent) {
    taskCount += 1
    jobStatistics(e.jobPath).taskCount += 1
  }
}

private object ManyJobsIT {
  private val n = 50
  private val tasksPerSecond = 2.0
  private val interval = standardSeconds((n / tasksPerSecond).toInt)

  case class JobDefinition(name: Int) {
    def path = JobPath.of(s"/a-$name")

    def xmlElem =
      <job name={path.getName} enabled="false">
        <script java_class="com.sos.scheduler.engine.test.jobs.SingleStepJob"/>
        <run_time>
          <period absolute_repeat={interval.getStandardSeconds.toString}/>
        </run_time>
      </job>
  }

  case class JobStatistics(var taskCount: Int = 0)
}
