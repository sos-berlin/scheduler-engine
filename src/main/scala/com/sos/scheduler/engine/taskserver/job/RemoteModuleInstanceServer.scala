package com.sos.scheduler.engine.taskserver.job

import com.sos.scheduler.engine.common.scalautil.Logger
import com.sos.scheduler.engine.minicom.IUnknownFactory
import com.sos.scheduler.engine.minicom.types.{CLSID, EmptyVariant, IDispatch, IID, IUnknown, VariantArray}
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
    for (KeyValueRegex(key, value) ← arguments.indexedSeq filter { _ != EmptyVariant } map { _.asInstanceOf[String] }
         if value.nonEmpty) {
      key match {
        case k if KeyNames contains k ⇒ argMap += k → value
        case k ⇒ logger.debug(s"RemoteModuleInstanceServer.construct: Unsupported key: $k=$value")
      }
    }
  }

  def begin(objectAnys: VariantArray, objectNamesAnys: VariantArray): Unit = {
    val objectNames = objectNamesAnys.indexedSeq map { _.asInstanceOf[String] }
    val objects = objectAnys.indexedSeq map { _.asInstanceOf[IUnknown] }
    require(objectNames.size == objects.size)
    val namedObjects = objectNames zip objects

    val script = Script.parseXmlString(argMap("script"))

    task = argMap("language") match {
      case "shell" ⇒ new ShellScriptTask(namedObjects.toMap, script = script.string)
    }
    task.start()
  }

  def end(succeeded: Boolean): Any = {
    task.end()
  }

  def step(): Any = {
    task.step()
  }

  def waitForSubprocesses(): Unit = {
  }
}

object RemoteModuleInstanceServer extends IUnknownFactory {
  private val logger = Logger(getClass)

  def clsid = CLSID(UUID.fromString("feee47a3-6c1b-11d8-8103-000476ee8afb"))
  def iid = IID(UUID.fromString("feee47a2-6c1b-11d8-8103-000476ee8afb"))
  def apply() = new RemoteModuleInstanceServer

  private val KeyValueRegex = "([^=]+)=(.*)".r
  private val KeyNames = Set(
    "language",
    //"com_class",
    "filename",
    "java_class",
    "recompile",
    "script",
    "job",
    "task_id",
    "environment",
    "has_order",
    "process.filename",
    "process.param_raw",
    "process.log_filename",
    "process.ignore_error",
    "process.ignore_signal",
    "process.shell_variable_prefix",
    "monitor.language",
    "monitor.name",
    "monitor.ordering",
    //"monitor.com_class",
    "monitor.filename",
    "monitor.java_class",
    "monitor.recompile",
    "monitor.script")
}
