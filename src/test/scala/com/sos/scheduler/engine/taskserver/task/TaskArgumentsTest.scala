package com.sos.scheduler.engine.taskserver.task

import com.sos.scheduler.engine.data.job.TaskId
import com.sos.scheduler.engine.minicom.types.VariantArray
import com.sos.scheduler.engine.taskserver.module.Script
import com.sos.scheduler.engine.taskserver.module.java.JavaModule
import org.junit.runner.RunWith
import org.scalatest.FreeSpec
import org.scalatest.junit.JUnitRunner

/**
 * @author Joacim Zschimmer
 */
@RunWith(classOf[JUnitRunner])
final class TaskArgumentsTest extends FreeSpec {

  private val taskArguments = TaskArguments(VariantArray(Vector(
    "language=shell",
    "job=JOBNAME",
    "task_id=123",
    "has_order=1",
    "script=" + <source><source_part linenr="100">PART-A
</source_part><source_part linenr="200">PART-B</source_part></source>,
    "environment=" + <sos.spooler.variable_set><variable name="A" value="a"/></sos.spooler.variable_set>,
    "monitor.language=java",
    "monitor.name=MONITOR-NAME",
    "monitor.java_class=com.example.A",
    "monitor.ordering=7",
    "monitor.script=",
    "monitor.language=java",
    "monitor.java_class=com.example.B",
    "monitor.script=",
    "monitor.language=java",
    "monitor.java_class=com.example.C",
    "monitor.script="
  )))

  "jobName" in {
    assert(taskArguments.jobName == "JOBNAME")
  }

  "taskId" in {
    assert(taskArguments.taskId == TaskId(123))
  }

  "moduleLanguage" in {
    taskArguments.moduleLanguage
  }

  "script" in {
    assert(taskArguments.script == Script("PART-A\nPART-B"))
  }

  "hasOrder" in {
    assert(taskArguments.hasOrder)
  }

  "environment" in {
    assert(taskArguments.environment == Map("A" → "a"))
  }

  "monitors" in {
    assert(taskArguments.monitors == List(
      Monitor(JavaModule("com.example.B"), name = "", ordering = 1),
      Monitor(JavaModule("com.example.C"), name = "", ordering = 1),
      Monitor(JavaModule("com.example.A"), name = "MONITOR-NAME", ordering = 7)
    ))
  }
}
