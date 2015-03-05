package com.sos.scheduler.engine.taskserver.task

import com.sos.scheduler.engine.common.scalautil.Closers.implicits._
import com.sos.scheduler.engine.common.scalautil.FileUtils.implicits._
import com.sos.scheduler.engine.common.scalautil.HasCloser
import com.sos.scheduler.engine.taskserver.module.ShellModuleInstance
import com.sos.scheduler.engine.taskserver.task.ShellProcessTask._
import com.sos.scheduler.engine.taskserver.task.common.VariableSets
import com.sos.scheduler.engine.taskserver.task.process.{ShellProcess, ShellProcessStarter}
import java.nio.charset.StandardCharsets._
import java.nio.file.Files._

/**
 * @author Joacim Zschimmer
 */
final class ShellProcessTask(moduleInstance: ShellModuleInstance, environment: Map[String, String], jobName: String, hasOrder: Boolean)
extends Task with HasCloser {

  private var process: ShellProcess = null
  private lazy val orderParamsFile = createTempFile("sos-", ".tmp")

  override def start() = {
    val params = moduleInstance.spoolerTask.parameterMap ++ moduleInstance.spoolerTask.orderParameterMap
    val paramEnv = params map { case (k, v) ⇒ s"$EnvironmentParameterPrefix$k" → v }
    process = ShellProcessStarter.start(
      name = jobName,
      additionalEnvironment = environment + (ReturnValuesFileEnvironmentVariableName → orderParamsFile.toAbsolutePath.toString) ++ paramEnv,
      scriptString = moduleInstance.module.script.string.trim)
    closer.registerAutoCloseable(process)
  }

  override def end() = {}

  override def step() = {
    val resultCode = process.waitForTermination(logOutputLine = moduleInstance.spoolerLog.info)
    transferReturnValuesToMaster()
    <process.result
      state_text={process.firstStdoutLine}
      spooler_process_result="true"
      exit_code={resultCode.value.toString}/>.toString()
  }

  private def transferReturnValuesToMaster(): Unit = {
    val variables = fetchReturnValues()
    if (variables.nonEmpty) {
      val xmlString = VariableSets.toXmlElem(fetchReturnValues()).toString()
      if (hasOrder)
        moduleInstance.spoolerTask.orderParamsXml = xmlString
      else
        moduleInstance.spoolerTask.paramsXml = xmlString
    }
  }

  private def fetchReturnValues(): Map[String, String] =
    (io.Source.fromFile(orderParamsFile)(ReturnValuesFileEncoding).getLines map { _.trim } collect {
      case ReturnValuesRegex(name, value) ⇒ name.trim → value.trim
      case line ⇒ throw new IllegalArgumentException(s"Not the expected syntax NAME=VALUE in file denoted by environment variable $ReturnValuesFileEnvironmentVariableName: $line")
    }).toMap

  def files = {
    if (process == null) throw new IllegalStateException
    process.files
  }
}

object ShellProcessTask {
  private val EnvironmentParameterPrefix = "SCHEDULER_PARAM_"
  private val ReturnValuesFileEnvironmentVariableName = "SCHEDULER_RETURN_VALUES"
  private val ReturnValuesFileEncoding = ISO_8859_1  // For v1.9 (and later ???)
  private val ReturnValuesRegex = "([^=]+)=(.*)".r
}
