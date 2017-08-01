package com.sos.scheduler.engine.kernel.processclass

import com.sos.scheduler.engine.agent.data.commands.StartTask
import com.sos.scheduler.engine.client.agent.ApiProcessConfiguration
import com.sos.scheduler.engine.common.guice.GuiceImplicits._
import com.sos.scheduler.engine.common.scalautil.Logger
import com.sos.scheduler.engine.common.scalautil.ScalazStyle.OptionRichBoolean
import com.sos.scheduler.engine.common.time.ScalaTime._
import com.sos.scheduler.engine.cplusplus.runtime.annotation.ForCpp
import com.sos.scheduler.engine.cplusplus.runtime.{Sister, SisterType}
import com.sos.scheduler.engine.data.filebased.FileBasedType
import com.sos.scheduler.engine.data.job.{JobPath, TaskId}
import com.sos.scheduler.engine.data.processclass.ProcessClassPath
import com.sos.scheduler.engine.kernel.async.{CppCall, SchedulerThreadCallQueue}
import com.sos.scheduler.engine.kernel.cppproxy.{Api_process_configurationC, Process_classC, SpoolerC}
import com.sos.scheduler.engine.kernel.filebased.FileBased
import com.sos.scheduler.engine.kernel.processclass.ProcessClass._
import com.sos.scheduler.engine.kernel.processclass.agent.{Agent, CppHttpRemoteApiProcessClient}
import com.sos.scheduler.engine.kernel.processclass.common.FailableCollection
import com.sos.scheduler.engine.kernel.scheduler.HasInjector
import java.time.Duration
import org.scalactic.Requirements._
import org.w3c.dom
import scala.collection.{immutable, mutable}
import scala.math.max

@ForCpp
final class ProcessClass private(
  protected[this] val cppProxy: Process_classC,
  protected val subsystem: ProcessClassSubsystem,
  callQueue: SchedulerThreadCallQueue,
  newCppHttpRemoteApiProcessClient: CppHttpRemoteApiProcessClient.Factory,
  /** Verzögerung für nicht erreichbare Agents - erst nach Scheduler-Aktivierung (Settings::freeze) nutzbar. */
  val agentConnectRetryDelayLazy: () ⇒ Duration)
extends FileBased {

  type Path = ProcessClassPath

  private[this] var _config = Configuration(None, immutable.IndexedSeq())
  private[this] var _failableAgents: FailableCollection[Agent] = null
  private[this] val clients = mutable.HashSet[CppHttpRemoteApiProcessClient]()

  def stringToPath(o: String) = ProcessClassPath(o)

  def fileBasedType = FileBasedType.processClass

  def onCppProxyInvalidated(): Unit = {
    for (c ← clients) logger.error(s"CppHttpRemoteApiProcessClient has not been removed: $c")
  }

  @ForCpp
  def processConfigurationDomElement(element: dom.Element): Unit = {
    changeConfiguration(Configuration.parse(element))
  }

  @ForCpp
  def replaceWith(other: ProcessClass): Unit = {
    changeConfiguration(other.config)
  }

  private def changeConfiguration(c: Configuration): Unit = {
    _config = c
    _failableAgents = null
    if (_config.agents.nonEmpty) {
      _failableAgents = new FailableCollection(_config.agents, agentConnectRetryDelayLazy, c.selectionMethod)
      for (c ← clients) {
        c.changeFailableAgents(_failableAgents)
      }
    } else {
      // Kein remote_scheduler mehr? Dann brechen wir alle auf einen remote_scheduler wartendenen Tasks ab. C++ wird sie nicht neu starten.
      for (c ← clients) {
        c.stopTaskWaitingForAgent()
      }
    }
  }

  @ForCpp
  def hasMoreAgents = config.moreAgents.nonEmpty

  @ForCpp
  def startCppHttpRemoteApiProcessClient(
    conf: Api_process_configurationC,
    schedulerApiTcpPort: Int,
    warningCall: CppCall,
    resultCall: CppCall): CppHttpRemoteApiProcessClient =
  {
    val r = newCppHttpRemoteApiProcessClient(apiProcessConfiguration(conf), schedulerApiTcpPort, warningCall, resultCall)
    clients += r
    r.startRemoteTask(failableAgents)
    r
  }

  @ForCpp
  def removeCppHttpRemoteApiProcessClient(client: CppHttpRemoteApiProcessClient): Unit = {
    try client.close()
    finally clients -= client
  }

  def agents: immutable.Seq[Agent] = config.agents

  private def config = {
    requireNonNull(_config)
    _config
  }

  private def failableAgents = {
    requireNonNull(_failableAgents)
    _failableAgents
  }
}

object ProcessClass {
  private val logger = Logger(getClass)

  final class Type extends SisterType[ProcessClass, Process_classC] {
    def sister(proxy: Process_classC, context: Sister) = {
      val injector = context.asInstanceOf[HasInjector].injector
      new ProcessClass(
        proxy,
        injector.instance[ProcessClassSubsystem],
        injector.instance[SchedulerThreadCallQueue],
        injector.instance[CppHttpRemoteApiProcessClient.Factory],
        agentConnectRetryDelayLazy = () ⇒ max(1, injector.instance[SpoolerC].settings()._remote_scheduler_connect_retry_delay).s)
    }
  }

  private def apiProcessConfiguration(c: Api_process_configurationC) = {
    new ApiProcessConfiguration(
      meta = StartTask.Meta(JobPath(c._job_path), TaskId(c._task_id)),
      hasApi = c._has_api,
      javaOptions = c._java_options.trim,
      javaClasspath = c._java_classpath.trim,
      logon = c._credentials_key.trim.nonEmpty option
        StartTask.KeyLogon(
          credentialsKey = c._credentials_key.trim,
          withUserProfile = c._load_user_profile))
  }
}
