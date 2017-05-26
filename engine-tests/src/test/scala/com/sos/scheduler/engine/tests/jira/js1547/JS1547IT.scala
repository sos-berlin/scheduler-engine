package com.sos.scheduler.engine.tests.jira.js1547

import com.sos.jobscheduler.data.job.TaskId
import com.sos.scheduler.engine.data.job.JobPath
import com.sos.scheduler.engine.data.processclass.ProcessClassPath
import com.sos.scheduler.engine.test.SchedulerTestUtils._
import com.sos.scheduler.engine.test.configuration.TestConfiguration
import com.sos.scheduler.engine.test.scalatest.ScalaSchedulerTest
import org.junit.runner.RunWith
import org.scalatest.FreeSpec
import org.scalatest.junit.JUnitRunner

/**
  * @author Joacim Zschimmer
  */
@RunWith(classOf[JUnitRunner])
final class JS1547IT extends FreeSpec with ScalaSchedulerTest {

  override protected lazy val testConfiguration = TestConfiguration(getClass, ignoreError = _ ⇒ true)

  "show_state shows process in process class" in {
    val jobPaths = List(JobPath("/test-shell"), JobPath("/test-api"))
    val runs = for (j ← jobPaths) yield startJob(j)
    for (run ← runs) awaitSuccess(run.started)

    // JS-1613: ProcessClasssDetailed should contain the pid
    val defaultClassProcesses = processClassDetailed(ProcessClassPath.Default).processes
    assert(defaultClassProcesses.size == 2 && (defaultClassProcesses forall { o ⇒ o.pid.nonEmpty && o.pid != Some(0) }))

    val answer = (scheduler executeXml <show_state subsystems='process_class' what='folders'/>).answer
    assert(runs forall { o ⇒ ! o.ended.isCompleted })
    //println(answer)
    val processes = answer \ "state" \ "folder" \ "process_classes" \ "process_class" \ "processes" \ "process"
    for ((jobPath, taskId) ← runs map { o ⇒ o.jobPath → o.taskId }) scheduler executeXml <kill_task job={jobPath.string} id={taskId.string}/>
    assert((processes flatMap { _.attribute("job") map { o ⇒ JobPath(o.toString) }}).toSet == jobPaths.toSet)
    assert((processes flatMap { _.attribute("task_id") map { o ⇒ TaskId(o.toString.toInt) }}).toSet == (runs map { _.taskId }).toSet)
    for (p ← defaultClassProcesses) scheduler executeXml <kill_task id={p.taskId.string} job={p.jobPath} immediately="true"/>
  }
}
