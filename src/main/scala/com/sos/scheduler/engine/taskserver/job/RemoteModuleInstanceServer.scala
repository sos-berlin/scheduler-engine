package com.sos.scheduler.engine.taskserver.job

import com.sos.scheduler.engine.common.scalautil.Logger
import com.sos.scheduler.engine.common.scalautil.ScalaUtils.cast
import com.sos.scheduler.engine.minicom.IUnknownFactory
import com.sos.scheduler.engine.minicom.types.{CLSID, IDispatch, IID, Variant, VariantArray}
import com.sos.scheduler.engine.taskserver.task.{ShellScriptTask, Task}
import java.util.UUID
import org.scalactic.Requirements._
import scala.collection.mutable

/**
 * @author Joacim Zschimmer
 */
final class RemoteModuleInstanceServer extends IDispatch {
  import com.sos.scheduler.engine.taskserver.job.RemoteModuleInstanceServer._

  private val argMap = mutable.Map[String, String]()
  private var task: Task = null

  def construct(arguments: VariantArray): Unit = {
    for (KeyValueRegex(key, value) ← arguments.indexedSeq filter { _ != Variant.BoxedEmpty } map cast[String]
         if value.nonEmpty) {
      key match {
        case k if KeySet contains k ⇒ argMap += k → value
        case k ⇒ logger.debug(s"RemoteModuleInstanceServer.construct: Unsupported key: $k=$value")
      }
    }
  }

  def begin(objectAnys: VariantArray, objectNamesAnys: VariantArray): Unit = {
    val objectNames = objectNamesAnys.as[String]
    val objects = objectAnys.asIUnknowns
    require(objectNames.size == objects.size)
    val namedObjects = objectNames zip objects
    val script = Script.parseXmlString(argMap(keys.Script))
    task = argMap(keys.Language) match {
      case "shell" ⇒ new ShellScriptTask(namedObjects.toMap, script = script.string)
      case o ⇒ throw new IllegalArgumentException(s"Unknown language '$o'")
    }
    task.start()
  }

  def end(succeeded: Boolean): Any = {
    if (task != null)
      task.end()
  }

  def step(): Any =
    task.step()

  def waitForSubprocesses(): Unit = {}

  override def toString =
    s"${getClass.getSimpleName}" + List(
      argMap get keys.Job map { o ⇒ s"job $o" },
      argMap get keys.TaskId map { o ⇒ s":$o" })
      .flatten.mkString("(", "", ")")
}

object RemoteModuleInstanceServer extends IUnknownFactory {
  private val logger = Logger(getClass)
  val clsid = CLSID(UUID.fromString("feee47a3-6c1b-11d8-8103-000476ee8afb"))
  val iid = IID(UUID.fromString("feee47a2-6c1b-11d8-8103-000476ee8afb"))

  def apply() = new RemoteModuleInstanceServer

  private val KeyValueRegex = "([^=]+)=(.*)".r  // "key=value"
  object keys {
    val Language = "language"
    //"com_class",
    //val Filename = "filename"
    //val Java_class = "java_class"
    //val Recompile = "recompile"
    val Script = "script"
    val Job = "job"
    val TaskId = "task_id"
    //val Environment = "environment"
    //val HasOrder = "has_order"
    //val ProcessFilename = "process.filename"
    //val ProcessParam_raw = "process.param_raw"
    //val ProcessLog_filename = "process.log_filename"
    //val ProcessIgnore_error = "process.ignore_error"
    //val ProcessIgnore_signal = "process.ignore_signal"
    //val ProcessShellVariablePrefix = "process.shell_variable_prefix"
    //val MonitorLanguage = "monitor.language"
    //val MonitorName = "monitor.name"
    //val MonitorOrdering = "monitor.ordering"
    //val MonitorComClass = "monitor.com_class"
    //val MonitorFilename = "monitor.filename"
    //val MonitorJavaClass = "monitor.java_class"
    //val MonitorRecompile = "monitor.recompile"
    //val MonitorScript = "monitor.script"
  }
  private val KeySet = Set(
    keys.Language,
    keys.Script,
    keys.Job,
    keys.TaskId)
}
