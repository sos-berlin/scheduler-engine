package com.sos.scheduler.engine.tests.jira.js1026

import com.google.common.collect.ImmutableList
import com.sos.jobscheduler.data.event.KeyedEvent
import com.sos.scheduler.engine.data.job.{TaskDetailed, TaskEnded, TaskKey}
import com.sos.scheduler.engine.test.SchedulerTestUtils.taskDetailed
import com.sos.scheduler.engine.test.scalatest.ScalaSchedulerTest
import com.sos.scheduler.engine.test.util.CommandBuilder
import com.sos.scheduler.engine.tests.jira.js1026.JS1026ShellJobIT.{assertObject, jobNames}
import org.junit.Assert.assertTrue
import org.junit.runner.RunWith
import org.scalatest.FreeSpec
import org.scalatest.junit.JUnitRunner
import scala.collection.JavaConversions._

/**
  * Test for SCHEDULER_RETURN_VALUES in standalone shell jobs.
  * The standalone jobs test1 and test2 will be started.
  * job test1 starts job test2 with command start_job.
  *
  * @author Florian Schreiber
  */
@RunWith(classOf[JUnitRunner])
final class JS1026ShellJobIT extends FreeSpec with ScalaSchedulerTest {
  private val util = new CommandBuilder
  private var taskCount: Int = 0

  "test" in {
    for (jobName ← jobNames) {
      controller.scheduler.executeXml(util.startJobImmediately(jobName).getCommand)
    }
  }

  eventBus.onHot[TaskEnded] {
    case KeyedEvent(TaskKey(_, taskId), _) ⇒
      val task = taskDetailed(taskId)
      taskCount += 1
      // Job test2 is started by command start_job of job test1
      if (taskCount == jobNames.size + 1) {
        assertObject(task, "testvar1", "value1")
        assertObject(task, "testvar2", "newvalue2")
        assertObject(task, "testvar3", "value3")
        assertObject(task, "testvar4", "newvalue4")
        assertObject(task, "testvar5", "value5")
        controller.terminateScheduler()
      }
  }
}

private object JS1026ShellJobIT {
  private val jobNames: ImmutableList[String] = ImmutableList.of("test1")

  /**
    * checks if an estimated object was given
    */
  private def assertObject(task: TaskDetailed, name: String, expected: String) {
    val value = task.variables(name)
    assertTrue(s"$name is not set in scheduler variables", value != "")
    assertTrue(s"$name=$value is not valid - $expected expected", value == expected)
  }
}

