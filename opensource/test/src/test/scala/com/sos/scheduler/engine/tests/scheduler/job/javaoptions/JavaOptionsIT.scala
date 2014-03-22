package com.sos.scheduler.engine.tests.scheduler.job.javaoptions

import JavaOptionsIT._
import com.sos.scheduler.engine.data.job.{JobPath, TaskClosedEvent}
import com.sos.scheduler.engine.kernel.scheduler.SchedulerConfiguration
import com.sos.scheduler.engine.kernel.variable.VariableSet
import com.sos.scheduler.engine.test.configuration.TestConfiguration
import com.sos.scheduler.engine.test.scala.ScalaSchedulerTest
import com.sos.scheduler.engine.test.scala.SchedulerTestImplicits._
import com.sos.scheduler.engine.test.util.Sockets.findAvailablePort
import org.junit.runner.RunWith
import org.scalatest.FunSuite
import org.scalatest.Matchers._
import org.scalatest.junit.JUnitRunner

@RunWith(classOf[JUnitRunner])
final class JavaOptionsIT extends FunSuite with ScalaSchedulerTest {

  protected override lazy val testConfiguration = TestConfiguration(
    testClass = getClass,
    mainArguments = List("-tcp-port="+ findAvailablePort()))

  private val eventPipe = controller.newEventPipe()

  test("Local job should respect java_options") {
    runJob(localJobPath)
  }

  test("Remote job should respect java_options") {
    val h = "localhost:"+ instance[SchedulerConfiguration].tcpPort
    scheduler executeXml <process_class name="remote" remote_scheduler={h}/>
    runJob(remoteJobPath)
  }

  private def runJob(j: JobPath) {
    scheduler executeXml <start_job job={j.string}/>
    eventPipe.nextWithCondition { e: TaskClosedEvent => e.jobPath == j }
    instance[VariableSet].apply(j.name +".myJavaOption") should equal ("TEST")
  }
}


private object JavaOptionsIT {
  private val localJobPath = JobPath("/testLocal")
  private val remoteJobPath = JobPath("/testRemote")
}
