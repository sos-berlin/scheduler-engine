package com.sos.scheduler.engine.taskserver.job

import com.sos.scheduler.engine.common.scalautil.Closers.implicits._
import com.sos.scheduler.engine.common.scalautil.ScalaUtils.cast
import com.sos.scheduler.engine.common.scalautil.{HasCloser, Logger}
import com.sos.scheduler.engine.data.job.TaskId
import com.sos.scheduler.engine.minicom.IDispatchFactory
import com.sos.scheduler.engine.minicom.annotations.invocable
import com.sos.scheduler.engine.minicom.types.{CLSID, IDispatchable, IID, Variant, VariantArray}
import com.sos.scheduler.engine.taskserver.task.{NamedObjects, ScriptLanguage, ShellProcessTask, ShellScriptLanguage, Task, TaskConfiguration}
import java.util.UUID
import org.scalactic.Requirements._
import scala.collection.mutable

/**
 * @author Joacim Zschimmer
 * @see Com_remote_module_instance_server, spooler_module_remote_server.cxx
 */
final class RemoteModuleInstanceServer extends IDispatchable with HasCloser {
  import com.sos.scheduler.engine.taskserver.job.RemoteModuleInstanceServer._

  private var conf: TaskConfiguration = null
  private var task: Task = null

  @invocable
  def construct(arguments: VariantArray): Unit = {
    val argMap = mutable.Map[String, String]()
    for (keyValueString ← arguments.indexedSeq filter { _ != Variant.BoxedEmpty } map cast[String]) {
      val KeyValueRegex(key, value) = keyValueString
      if (value.nonEmpty) {
        key match {
          case k if KeySet contains k ⇒ argMap += k → value
          case k ⇒ logger.debug(s"Ignoring unsupported key: $k=$value")
        }
      }
    }
    conf = toTaskConfiguration(argMap.toMap)
  }

  @invocable
  def begin(objectAnys: VariantArray, objectNamesAnys: VariantArray): Unit = {
    val namedObjectMap = toNamedObjectMap(names = objectNamesAnys, anys = objectAnys)
    val log = namedObjectMap.spoolerLog.info _
    task = languageToTask(conf.language, log).closeWithCloser
    task.start()
  }

  private def languageToTask(language: ScriptLanguage, log: String ⇒ Unit): Task =
    conf.language match {
      case ShellScriptLanguage ⇒ new ShellProcessTask(conf, log)
      case o ⇒ throw new IllegalArgumentException(s"Unknown language '$o'")
    }

  @invocable
  def end(succeeded: Boolean): Unit = {
    if (task != null)
      task.end()
  }

  @invocable
  def step() = task.step()

  @invocable
  def waitForSubprocesses(): Unit = {}

  override def toString =
    List(
      s"${getClass.getSimpleName}",
      Option(conf) map { t ⇒ s"(${t.jobName}:${t.taskId}})" })
      .mkString("")
}

object RemoteModuleInstanceServer extends IDispatchFactory {
  val clsid = CLSID(UUID.fromString("feee47a3-6c1b-11d8-8103-000476ee8afb"))
  val iid = IID(UUID.fromString("feee47a2-6c1b-11d8-8103-000476ee8afb"))
  private val logger = Logger(getClass)

  def apply() = new RemoteModuleInstanceServer

  private val KeyValueRegex = "(?s)([[a-z_.]]+)=(.*)".r  //  "(?s)" dot matches \n too, "key=value"
    val LanguageKey = "language"
    //"com_class",
    //val FilenameKey = "filename"
    //val Java_classKey = "java_class"
    //val RecompileKey = "recompile"
    val ScriptKey = "script"
    val JobKey = "job"
    val TaskIdKey = "task_id"
    //val EnvironmentKey = "environment"
    //val HasOrderKey = "has_order"
    //val ProcessFilenameKey = "process.filename"
    //val ProcessParam_rawKey = "process.param_raw"
    //val ProcessLog_filenameKey = "process.log_filename"
    //val ProcessIgnore_errorKey = "process.ignore_error"
    //val ProcessIgnore_signalKey = "process.ignore_signal"
    //val ProcessShellVariablePrefixKey = "process.shell_variable_prefix"
    //val MonitorLanguageKey = "monitor.language"
    //val MonitorNameKey = "monitor.name"
    //val MonitorOrderingKey = "monitor.ordering"
    //val MonitorComClassKey = "monitor.com_class"
    //val MonitorFilenameKey = "monitor.filename"
    //val MonitorJavaClassKey = "monitor.java_class"
    //val MonitorRecompileKey = "monitor.recompile"
    //val MonitorScriptKey = "monitor.script"
  private val KeySet = Set(
    LanguageKey,
    ScriptKey,
    JobKey,
    TaskIdKey)

  private def toNamedObjectMap(names: VariantArray, anys: VariantArray): NamedObjects = {
    val nameStrings = names.as[String]
    val iDispatches = anys.asIDispatch map { _.asInstanceOf[IDispatchable] }
    require(nameStrings.size == iDispatches.size)
    NamedObjects(nameStrings zip iDispatches)
  }

  private def toTaskConfiguration(args: Map[String, String]) =
    TaskConfiguration(
      jobName = args(JobKey),
      taskId = TaskId(args(TaskIdKey).toInt),
      language = ScriptLanguage(args(LanguageKey)),
      script = Script.parseXmlString(args(ScriptKey)).string
    )
}
