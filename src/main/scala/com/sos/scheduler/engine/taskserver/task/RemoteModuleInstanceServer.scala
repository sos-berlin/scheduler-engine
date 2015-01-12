package com.sos.scheduler.engine.taskserver.task

import com.sos.scheduler.engine.common.scalautil.Closers.implicits._
import com.sos.scheduler.engine.common.scalautil.ScalaUtils.cast
import com.sos.scheduler.engine.common.scalautil.xmls.ScalaXMLEventReader
import com.sos.scheduler.engine.common.scalautil.{HasCloser, Logger}
import com.sos.scheduler.engine.data.job.TaskId
import com.sos.scheduler.engine.minicom.idispatch.{IDispatchFactory, IDispatchable, invocable}
import com.sos.scheduler.engine.minicom.types.{CLSID, IID, VariantArray, variant}
import java.util.UUID
import org.scalactic.Requirements._
import scala.collection.{immutable, mutable}

/**
 * @author Joacim Zschimmer
 * @see Com_remote_module_instance_server, spooler_module_remote_server.cxx
 */
final class RemoteModuleInstanceServer extends IDispatchable with HasCloser {
  import com.sos.scheduler.engine.taskserver.task.RemoteModuleInstanceServer._

  private var argMap = mutable.Map[String, String]()
  private var conf: TaskConfiguration = null
  private var task: Task = null

  @invocable
  def construct(arguments: VariantArray): Unit = {
    for (keyValueString ← arguments.indexedSeq filterNot variant.isEmpty map cast[String]) {
      val KeyValueRegex(key, value) = keyValueString
      if (value.nonEmpty) {
        key match {
          case k if KeySet contains k ⇒ argMap += k → value
          case k ⇒ logger.debug(s"Ignoring unsupported key: $k=$value")
        }
      }
    }
  }

  @invocable
  def begin(objectAnys: VariantArray, objectNamesAnys: VariantArray): Unit = {
    val namedObjectMap = toNamedObjectMap(names = objectNamesAnys, anys = objectAnys)
    val spoolerLog = namedObjectMap.spoolerLog
    val spoolerTask = namedObjectMap.spoolerTask
    conf = toTaskConfiguration(
        argMap.toMap,
        params = parseVariableSet(spoolerTask.paramsXml) ++ (spoolerTask.orderParamsXml match {
          case "" ⇒ Map()
          case o ⇒ parseVariableSet(o)
        }))
    task = languageToTask(conf.language, spoolerLog.info).closeWithCloser

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
  val clsid = CLSID(UUID fromString "feee47a3-6c1b-11d8-8103-000476ee8afb")
  val iid   = IID  (UUID fromString "feee47a2-6c1b-11d8-8103-000476ee8afb")
  private val EnvironmentParameterPrefix = "SCHEDULER_PARAM_"
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
    val EnvironmentKey = "environment"
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
    TaskIdKey,
    EnvironmentKey)

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

  private def toTaskConfiguration(args: Map[String, String], params: Map[String, String]) =
    TaskConfiguration(
      jobName = args(JobKey),
      taskId = TaskId(args(TaskIdKey).toInt),
      extraEnvironment = parseVariableSet(args(EnvironmentKey)) ++ (params map { case (k, v) ⇒ s"$EnvironmentParameterPrefix$k" -> v }),
      language = ScriptLanguage(args(LanguageKey)),
      script = Script.parseXmlString(args(ScriptKey)).string.trim
    )

  private def parseVariableSet(string: String, groupName: String = "", elementName: String = "variable"): Map[String, String] =
    ScalaXMLEventReader.parseString(string) { eventReader ⇒
      import eventReader._
      val myGroupName = if (groupName.nonEmpty) groupName else peek.asStartElement.getName.getLocalPart
      parseElement(myGroupName) {
        attributeMap("count")
        parseEachRepeatingElement("variable") {
          attributeMap("name") → attributeMap.getOrElse("value", "")
        }
      }
    }.toMap
}
