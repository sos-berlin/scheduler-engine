package com.sos.scheduler.engine.taskserver.task

import com.sos.scheduler.engine.common.scalautil.AutoClosing.autoClosing
import com.sos.scheduler.engine.common.scalautil.FileUtils.implicits._
import com.sos.scheduler.engine.common.scalautil.xmls.SafeXML
import com.sos.scheduler.engine.common.system.OperatingSystem._
import com.sos.scheduler.engine.common.time.ScalaJoda._
import com.sos.scheduler.engine.data.job.TaskId
import com.sos.scheduler.engine.test.util.time.WaitForCondition.waitForCondition
import org.junit.runner.RunWith
import org.scalatest.FreeSpec
import org.scalatest.Matchers._
import org.scalatest.junit.JUnitRunner
import scala.collection.mutable

/**
 * @author Joacim Zschimmer
 */
@RunWith(classOf[JUnitRunner])
final class ShellProcessTaskTest extends FreeSpec {
  "ShellProcessTask" in {
    val exitCode = 7
    val testName = "TESTENV"
    val testValue = "TESTENV-VALUE"
    val testString = "TEST-SCRIPT"
    val conf = TaskConfiguration(
      jobName = "TEST-JOB",
      taskId = TaskId(1),
      extraEnvironment = Map(testName → testValue),
      language = ShellScriptLanguage,
      script = (if (isWindows) s"@echo off\necho $testName=%$testName%" else s"echo $testName=$$$testName") + "\n" +
        s"echo $testString\n" +
        s"exit $exitCode")
    val outputLines = mutable.Buffer[String]()
    val (result, files) = autoClosing(new ShellProcessTask(conf, log = { o ⇒ outputLines += o })) { task ⇒
      task.start()
      (task.step(), task.files)
    }
    SafeXML.loadString(result) shouldEqual <process.result
      state_text={s"$testName=$testValue"}
      spooler_process_result="true"
      exit_code={exitCode.toString}/>
    assert(outputLines contains testString)
    assert(outputLines contains s"$testName=$testValue")
    waitForCondition(timeout = 3.s, step = 10.ms) { files forall { !_.exists }}  // Waiting for Future
    files filter { _.exists } match {
      case Nil ⇒
      case undeletedFiles ⇒ fail(s"Files not deleted:\n" + undeletedFiles.mkString("\n"))
    }
  }
}
