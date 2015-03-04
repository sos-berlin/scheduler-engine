package com.sos.scheduler.engine.taskserver.task

import com.sos.scheduler.engine.common.scalautil.Closers.implicits._
import com.sos.scheduler.engine.common.scalautil.FileUtils.implicits._
import com.sos.scheduler.engine.common.scalautil.ScalaUtils.cast
import com.sos.scheduler.engine.common.scalautil.{HasCloser, Logger}
import com.sos.scheduler.engine.data.job.TaskId
import com.sos.scheduler.engine.minicom.idispatch.{IDispatchFactory, IDispatchable, invocable}
import com.sos.scheduler.engine.minicom.types.{CLSID, IID, VariantArray, variant}
import com.sos.scheduler.engine.taskserver.spoolerapi.SpoolerTask
import com.sos.scheduler.engine.taskserver.task.common.VariableSets
import java.nio.charset.StandardCharsets.ISO_8859_1
import java.nio.file.Files.createTempFile
import java.nio.file.Path
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
  private lazy val orderParamsFile = createTempFile("sos-", ".tmp")
  private var spoolerTask: SpoolerTask = null

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
    spoolerTask = namedObjectMap.spoolerTask
    conf = toTaskConfiguration(
        argMap.toMap,
        params = VariableSets.parseXml(spoolerTask.paramsXml) ++
          (spoolerTask.orderParamsXml match {
            case "" ⇒ Map()
            case o ⇒ VariableSets.parseXml(o)
          }),
        orderParamsFile = orderParamsFile)
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
  def step(): String = {
    val result = task.step()
    transferReturnValuesToMaster()
    result
  }

  private def transferReturnValuesToMaster(): Unit = {
    val variables = fetchReturnValues()
    if (variables.nonEmpty) {
      val xmlString = VariableSets.toXmlElem(fetchReturnValues()).toString()
      if (conf.hasOrder)
        spoolerTask.orderParamsXml = xmlString
      else
        spoolerTask.paramsXml = xmlString
    }
  }

  private def fetchReturnValues(): Map[String, String] =
    (io.Source.fromFile(orderParamsFile)(ReturnValuesFileEncoding).getLines map { _.trim } collect {
      case ReturnValuesRegex(name, value) ⇒ name.trim → value.trim
      case line ⇒ throw new IllegalArgumentException(s"Not the expected syntax NAME=VALUE in file denoted by environment variable $ReturnValuesFileEnvironmentVariableName: $line")
    }).toMap

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
  private val ReturnValuesFileEnvironmentVariableName = "SCHEDULER_RETURN_VALUES"
  private val ReturnValuesFileEncoding = ISO_8859_1  // For v1.9 (and later ???)
  private val ReturnValuesRegex = "([^=]+)=(.*)".r
  private val logger = Logger(getClass)

  private val KeyValueRegex = "(?s)([[a-z_.]]+)=(.*)".r  //  "(?s)" dot matches \n too, "key=value"
  private val LanguageKey = "language"
  //TODO "com_class",
  //TODO private val FilenameKey = "filename"
  //TODO private val Java_classKey = "java_class"
  //TODO private val RecompileKey = "recompile"
  private val ScriptKey = "script"
  private val JobKey = "job"
  private val TaskIdKey = "task_id"
  private val EnvironmentKey = "environment"
  private val HasOrderKey = "has_order"
  //TODO private val ProcessFilenameKey = "process.filename"
  //TODO private val ProcessParam_rawKey = "process.param_raw"
  //TODO private val ProcessLog_filenameKey = "process.log_filename"
  //TODO private val ProcessIgnore_errorKey = "process.ignore_error"
  //TODO private val ProcessIgnore_signalKey = "process.ignore_signal"
  //TODO private val ProcessShellVariablePrefixKey = "process.shell_variable_prefix"
  //TODO private val MonitorLanguageKey = "monitor.language"
  //TODO private val MonitorNameKey = "monitor.name"
  //TODO private val MonitorOrderingKey = "monitor.ordering"
  //TODO private val MonitorComClassKey = "monitor.com_class"
  //TODO private val MonitorFilenameKey = "monitor.filename"
  //TODO private val MonitorJavaClassKey = "monitor.java_class"
  //TODO private val MonitorRecompileKey = "monitor.recompile"
  //TODO private val MonitorScriptKey = "monitor.script"
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

  private def toTaskConfiguration(args: Map[String, String], params: Map[String, String], orderParamsFile: Path) =
    TaskConfiguration(
      jobName = args(JobKey),
      taskId = TaskId(args(TaskIdKey).toInt),
      extraEnvironment = VariableSets.parseXml(args(EnvironmentKey)) ++
        (params map { case (k, v) ⇒ s"$EnvironmentParameterPrefix$k" → v }) +
        (ReturnValuesFileEnvironmentVariableName → orderParamsFile.toAbsolutePath.toString),
      language = ScriptLanguage(args(LanguageKey)),
      script = Script.parseXmlString(args(ScriptKey)).string.trim,
      hasOrder = args.get(HasOrderKey) match {
        case Some("1") ⇒ true
        case Some(o) ⇒ throw new IllegalArgumentException(s"Invalid TaskConfiguration: $HasOrderKey=$o")
        case None ⇒ false
      })
}
