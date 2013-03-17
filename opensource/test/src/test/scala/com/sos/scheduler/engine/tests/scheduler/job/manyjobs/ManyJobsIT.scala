package com.sos.scheduler.engine.tests.scheduler.job.manyjobs

import ManyJobsIT._
import com.sos.scheduler.engine.common.time.ScalaJoda.{DurationRichInt, sleep}
import com.sos.scheduler.engine.data.folder.{FileBasedActivatedEvent, JobPath}
import com.sos.scheduler.engine.data.job.TaskStartedEvent
import com.sos.scheduler.engine.eventbus.EventHandler
import com.sos.scheduler.engine.test.binary.CppBinariesDebugMode
import com.sos.scheduler.engine.test.scala.ScalaSchedulerTest
import com.sos.scheduler.engine.test.scala.SchedulerTestImplicits._
import com.sos.scheduler.engine.test.util.time.WaitForCondition.waitForCondition
import org.junit.runner.RunWith
import org.scalatest.junit.JUnitRunner
import scala.collection.mutable
import scala.math._
import scala.sys.error
import scala.util.Try

@RunWith(classOf[JUnitRunner])
class ManyJobsIT extends ScalaSchedulerTest {

  override val binariesDebugMode = if (n > 0) Some(CppBinariesDebugMode.release) else None
  private var activatedJobCount = 0
  private var taskCount = 0
  private lazy val jobs = 1 to n map JobDefinition
  private lazy val jobStatistics = mutable.HashMap[JobPath, JobStatistics]() ++ (jobs map { j => j.path -> JobStatistics() })

  override def checkedBeforeAll() {
    controller.useDatabase()    // Damit die History-Dateien nicht die File-handles aufbrauchen.
    controller.setLogCategories("java.stackTrace-")
  }

  test(s"Adding $n jobs") {
    for (j <- jobs) scheduler executeXml j.xmlElem
    waitForCondition(timeout=n*100.ms, step=100.ms) { activatedJobCount == n }
  }

  val duration = 30.s //millis(min(30000, (1000 * n / tasksPerSecond).toInt)) //4 * interval
  val expectedTaskCount = (duration.getMillis * tasksPerSecond / 1000).toInt

  test(s"$expectedTaskCount tasks should run in ${duration.getMillis}ms ($tasksPerSecond tasks/s)") {
    if (n == 0) pending
    else {
      scheduler executeXmls (
            jobs map { o =>
              <modify_job job={o.path.string} cmd="enable"/>
              <modify_job job={o.path.string} cmd="unstop"/>
            })
      sleep(duration)
      controller.getEventBus.dispatchEvents()
      System.err.println(s"$taskCount in ${duration.getStandardSeconds}s, ${1000 * taskCount.toDouble / duration.getMillis} tasks/s")
      //taskCount should be (expectedTaskCount plusOrMinus (0.2 * expectedTaskCount).toInt)
      //(for (j <- jobStatistics) j.taskCount should equal (tasksPer)
    }
  }

  @EventHandler def handle(e: FileBasedActivatedEvent) {
    Some(e.getTypedPath) collect { case p: JobPath if jobStatistics.keySet contains p => activatedJobCount += 1 }
  }

  @EventHandler def handle(e: TaskStartedEvent) {
    taskCount += 1
    jobStatistics(e.jobPath).taskCount += 1
  }
}

private object ManyJobsIT {
  private val propertyName = "ManyJobsIT"
  private val minimumTasks = 10

  private lazy val n = {
    val v = System.getProperty(propertyName, "0")
    Try(v.toInt) getOrElse error(s"System property $propertyName=$v not a number") match {
      case 0 => 0
      case o => max(o, minimumTasks)
    }
  }
  private val tasksPerSecond = 40.0
  //private lazy val interval = standardSeconds((n / tasksPerSecond).toInt)

  case class JobDefinition(name: Int) {
    def path = JobPath.of(s"/a-$name")

    def xmlElem =
      <job name={path.getName} enabled="false">
        <script language="shell">exit 0</script>
        <run_time repeat="1"/>
      </job>
//          <run_time>
//            <period absolute_repeat={interval.getStandardSeconds.toString}/>
//          </run_time>
  }

  case class JobStatistics(var taskCount: Int = 0)
}
