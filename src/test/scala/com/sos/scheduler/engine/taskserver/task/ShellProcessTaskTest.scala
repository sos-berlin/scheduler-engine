package com.sos.scheduler.engine.taskserver.task

import com.sos.scheduler.engine.common.scalautil.AutoClosing.autoClosing
import com.sos.scheduler.engine.common.scalautil.FileUtils.implicits._
import com.sos.scheduler.engine.common.scalautil.xmls.SafeXML
import com.sos.scheduler.engine.common.system.OperatingSystem._
import com.sos.scheduler.engine.common.time.ScalaJoda._
import com.sos.scheduler.engine.minicom.idispatch.IDispatchable
import com.sos.scheduler.engine.taskserver.module.NamedObjects.{SpoolerLogName, SpoolerTaskName}
import com.sos.scheduler.engine.taskserver.module.shell.{ShellModule, ShellModuleInstance}
import com.sos.scheduler.engine.taskserver.module.{NamedObjects, Script}
import com.sos.scheduler.engine.taskserver.spoolerapi.{SpoolerLog, SpoolerTask}
import com.sos.scheduler.engine.taskserver.task.ShellProcessTaskTest._
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
    val infoMessages = mutable.Buffer[String]()
    def newTask() = {
      object TestSpoolerLog extends SpoolerLog with IDispatchable {
        def info(message: String) = infoMessages += message
      }
      object TestSpoolerTask extends SpoolerTask with IDispatchable {
        def paramsXml = ""
        def paramsXml_=(o: String) = throw new NotImplementedError
        def orderParamsXml = ""
        def orderParamsXml_=(o: String) = throw new NotImplementedError
      }
      val namedObjects = NamedObjects(List(SpoolerLogName → TestSpoolerLog, SpoolerTaskName → TestSpoolerTask))
      val module = new ShellModule(TestScript)
      val moduleInstance = new ShellModuleInstance(module, namedObjects)
      new ShellProcessTask(moduleInstance, jobName = "TEST-JOB", hasOrder = false, environment = Map(TestName → TestValue))
    }
    val (result, files) = autoClosing(newTask()) { task ⇒
      task.start()
      val result = task.step()
      (result, task.files)
    }
    SafeXML.loadString(result) shouldEqual <process.result
      state_text={s"$TestName=$TestValue"}
      spooler_process_result="true"
      exit_code={ExitCode.toString}/>
    assert(infoMessages contains TestString)
    assert(infoMessages contains s"$TestName=$TestValue")
    waitForCondition(timeout = 3.s, step = 10.ms) { files forall { !_.exists }}  // Waiting for Future
    files filter { _.exists } match {
      case Nil ⇒
      case undeletedFiles ⇒ fail(s"Files not deleted:\n" + undeletedFiles.mkString("\n"))
    }
  }
}

private object ShellProcessTaskTest {
  private val ExitCode = 7
  private val TestName = "TESTENV"
  private val TestValue = "TESTENV-VALUE"
  private val TestString = "TEST-SCRIPT"
  private val TestScript = Script(
    (if (isWindows) s"@echo off\necho $TestName=%$TestName%" else s"echo $TestName=$$$TestName") +
      "\n" +
      s"echo $TestString\n" +
      s"exit $ExitCode")
}
