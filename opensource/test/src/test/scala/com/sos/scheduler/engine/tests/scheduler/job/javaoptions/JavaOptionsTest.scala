package com.sos.scheduler.engine.tests.scheduler.job.javaoptions

import com.sos.scheduler.engine.data.folder.JobPath
import com.sos.scheduler.engine.data.job.TaskClosedEvent
import com.sos.scheduler.engine.test.scala.ScalaSchedulerTest
import com.sos.scheduler.engine.test.scala.SchedulerTestImplicits._
import com.sos.scheduler.engine.test.util.Sockets.findAvailablePort
import org.junit.runner.RunWith
import org.scalatest.junit.JUnitRunner
import org.scalatest.matchers.ShouldMatchers._

@RunWith(classOf[JUnitRunner])
class JavaOptionsTest extends ScalaSchedulerTest {
  import JavaOptionsTest._

  override def checkedBeforeAll() {
    controller.activateScheduler("-tcp-port="+ findAvailablePort())
    super.beforeAll()
  }

  private val eventPipe = controller.newEventPipe()

  test("Local job should respect java_options") {
    runJob(localJobPath)
  }

  test("Remote job should respect java_options") {
    val h = "localhost:"+ scheduler.getTcpPort
    scheduler executeXml <process_class name="remote" remote_scheduler={h}/>
    runJob(remoteJobPath)
  }

  private def runJob(j: JobPath) {
    scheduler executeXml <start_job job={j.asString}/>
    eventPipe.nextWithCondition { e: TaskClosedEvent => e.getJobPath == j }
    scheduler.getVariables.get(j.getName +".myJavaOption") should equal ("TEST")
  }
}

object JavaOptionsTest {
  private val localJobPath = JobPath.of("/testLocal")
  private val remoteJobPath = JobPath.of("/testRemote")
}
