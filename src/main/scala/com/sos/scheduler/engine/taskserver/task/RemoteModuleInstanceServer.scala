package com.sos.scheduler.engine.taskserver.task

import com.sos.scheduler.engine.common.scalautil.Closers.implicits._
import com.sos.scheduler.engine.common.scalautil.ScalaUtils.cast
import com.sos.scheduler.engine.common.scalautil.{HasCloser, Logger}
import com.sos.scheduler.engine.data.job.TaskId
import com.sos.scheduler.engine.minicom.idispatch.{IDispatchFactory, IDispatchable, invocable}
import com.sos.scheduler.engine.minicom.types.{CLSID, IID, VariantArray, variant}
import com.sos.scheduler.engine.taskserver.module.{ModuleInstance, NamedObjects, Script, ScriptLanguage, ShellModule, ShellModuleInstance, ShellScriptLanguage}
import com.sos.scheduler.engine.taskserver.task.common.VariableSets
import java.util.UUID
import org.scalactic.Requirements._
import scala.collection.{immutable, mutable}

/**
 * @author Joacim Zschimmer
 * @see Com_remote_module_instance_server, spooler_module_remote_server.cxx
 */
final class RemoteModuleInstanceServer extends IDispatchable with HasCloser {
  import com.sos.scheduler.engine.taskserver.task.RemoteModuleInstanceServer._

  private var remoteArgs: RemoteArgs = null
  private var moduleInstance: ModuleInstance = null
  private var task: Task = null

  @invocable
  def construct(arguments: VariantArray): Unit = remoteArgs = RemoteArgs(arguments)

  @invocable
  def begin(objectAnys: VariantArray, objectNamesAnys: VariantArray): Unit = {
    moduleInstance = remoteArgs.scriptLanguage match {
      case ShellScriptLanguage ⇒
        new ShellModuleInstance(new ShellModule(remoteArgs.script), toNamedObjectMap(names = objectNamesAnys, anys = objectAnys))
    }
    task = newShellTask().closeWithCloser
    task.start()
  }

  private def newShellTask(): Task =
    moduleInstance match {
      case moduleInstance: ShellModuleInstance ⇒
        new ShellProcessTask(moduleInstance,
          jobName = remoteArgs.jobName,
          hasOrder = remoteArgs.hasOrder,
          environment = remoteArgs.environment)
      case _ ⇒
        throw new IllegalArgumentException(s"Unknown language '${moduleInstance.module.scriptLanguage }'")
    }

  @invocable
  def end(succeeded: Boolean): Unit = {
    if (task != null)
      task.end()
  }

  @invocable
  def step(): String = task.step()

  @invocable
  def waitForSubprocesses(): Unit = {}

  override def toString =
    List(
      s"${getClass.getSimpleName}",
      Option(remoteArgs) map { t ⇒ s"(${t.jobName}:${t.taskId}})" })
      .mkString("")
}

object RemoteModuleInstanceServer extends IDispatchFactory {
  val clsid = CLSID(UUID fromString "feee47a3-6c1b-11d8-8103-000476ee8afb")
  val iid   = IID  (UUID fromString "feee47a2-6c1b-11d8-8103-000476ee8afb")
  private val logger = Logger(getClass)

  private val KeyValueRegex = "(?s)([[a-z_.]]+)=(.*)".r  //  "(?s)" dot matches \n too, "key=value"
  private val LanguageKey = "language"
  //TODO "com_class",
  //JS-1295 @deprecated private val FilenameKey = "filename"
  //TODO private val Java_classKey = "java_class"
  private val ScriptKey = "script"
  private val JobKey = "job"
  private val TaskIdKey = "task_id"
  private val EnvironmentKey = "environment"
  private val HasOrderKey = "has_order"
  //JS-1295 @deprecated private val ProcessFilenameKey = "process.filename"
  //TODO private val ProcessParam_rawKey = "process.param_raw"
  //TODO private val ProcessLog_filenameKey = "process.log_filename"
  //TODO private val ProcessIgnore_errorKey = "process.ignore_error"
  //TODO private val ProcessIgnore_signalKey = "process.ignore_signal"
  //TODO private val ProcessShellVariablePrefixKey = "process.shell_variable_prefix"
  private val MonitorLanguageKey = "monitor.language"
  private val MonitorNameKey = "monitor.name"
  private val MonitorOrderingKey = "monitor.ordering"
  //TODO private val MonitorComClassKey = "monitor.com_class"
  //JS-1295 @deprecated private val MonitorFilenameKey = "monitor.filename"
  private val MonitorJavaClassKey = "monitor.java_class"
  val MonitorScriptKey = "monitor.script"
  private val KeySet = Set(
    LanguageKey,
    ScriptKey,
    JobKey,
    TaskIdKey,
    EnvironmentKey,
    HasOrderKey)

  def apply() = new RemoteModuleInstanceServer

  private def toNamedObjectMap(names: VariantArray, anys: VariantArray): NamedObjects = {
    val nameStrings = names.as[String]
    val iDispatches = variantArrayToIDispatchables(anys)
    require(nameStrings.size == iDispatches.size)
    NamedObjects(nameStrings zip iDispatches)
  }

  /**
   * Expects an VariantArray with Some[IUnknown]
   * @return IUnknowns interpreted as IDispatchable
   * @throws NullPointerException when an IDispatch is null.
   */
  private def variantArrayToIDispatchables(a: VariantArray): immutable.IndexedSeq[IDispatchable] =
    a.indexedSeq.asInstanceOf[immutable.IndexedSeq[Some[_]]] map { case Some(o) ⇒ cast[IDispatchable](o) }

  private class RemoteArgs(argMap: Map[String, String]) {
    lazy val scriptLanguage = ScriptLanguage(argMap(LanguageKey))
    lazy val script = Script.parseXmlString(argMap(ScriptKey))
    lazy val jobName = argMap(JobKey)
    lazy val taskId = TaskId(argMap(TaskIdKey).toInt)
    lazy val environment = VariableSets.parseXml(argMap(EnvironmentKey))
    lazy val hasOrder = argMap.get(HasOrderKey) match {
      case Some("1") ⇒ true
      case Some(o) ⇒ throw new IllegalArgumentException(s"Invalid agent argument: $HasOrderKey=$o")
      case None ⇒ false
    }
  }

  private object RemoteArgs {
    def apply(arguments: VariantArray): RemoteArgs = {
      val argBuffer = mutable.Buffer[(String, String)]()
      for (keyValueString ← arguments.indexedSeq filterNot variant.isEmpty map cast[String]) {
        val KeyValueRegex(key, value) = keyValueString
        if (value.nonEmpty) {
          key match {
            case k if KeySet contains k ⇒ argBuffer += k → value
            case k ⇒ logger.debug(s"Ignoring unsupported key: $k=$value")
          }
        }
      }
      new RemoteArgs(argBuffer.toMap)
    }
  }
}
