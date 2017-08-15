package com.sos.scheduler.engine.kernel.processclass

import com.sos.scheduler.engine.agent.data.commands.StartTask
import com.sos.scheduler.engine.base.utils.ScalazStyle.OptionRichBoolean
import com.sos.scheduler.engine.client.agent.ApiProcessConfiguration
import com.sos.scheduler.engine.common.guice.GuiceImplicits._
import com.sos.scheduler.engine.common.scalautil.Collections._
import com.sos.scheduler.engine.common.scalautil.Logger
import com.sos.scheduler.engine.common.time.ScalaTime._
import com.sos.scheduler.engine.common.xml.DomForScala._
import com.sos.scheduler.engine.cplusplus.runtime.annotation.ForCpp
import com.sos.scheduler.engine.cplusplus.runtime.{Sister, SisterType}
import com.sos.scheduler.engine.data.agent.AgentAddress
import com.sos.scheduler.engine.data.job.{JobPath, TaskId}
import com.sos.scheduler.engine.data.processclass.{ProcessClassDetailed, ProcessClassObstacle, ProcessClassOverview, ProcessClassPath, ProcessClassView, ProcessDetailed}
import com.sos.scheduler.engine.kernel.async.{CppCall, SchedulerThreadCallQueue}
import com.sos.scheduler.engine.kernel.cppproxy.{Api_process_configurationC, Process_classC, SpoolerC}
import com.sos.scheduler.engine.kernel.filebased.FileBased
import com.sos.scheduler.engine.kernel.processclass.ProcessClass._
import com.sos.scheduler.engine.kernel.processclass.agent.{Agent, CppHttpRemoteApiProcessClient}
import com.sos.scheduler.engine.kernel.processclass.common.FailableCollection
import com.sos.scheduler.engine.kernel.scheduler.HasInjector
import java.time.{Duration, Instant}
import org.scalactic.Requirements._
import org.w3c.dom
import scala.collection.{immutable, mutable}
import scala.math.max
import spray.http.Uri

@ForCpp
final class ProcessClass private(
  protected[kernel] val cppProxy: Process_classC,
  protected[kernel] val subsystem: ProcessClassSubsystem,
  callQueue: SchedulerThreadCallQueue,
  newCppHttpRemoteApiProcessClient: CppHttpRemoteApiProcessClient.Factory,
  /** Verzögerung für nicht erreichbare Agents - erst nach Scheduler-Aktivierung (Settings::freeze) nutzbar. */
  val agentConnectRetryDelayLazy: () ⇒ Duration)
extends FileBased {

  protected type Self = ProcessClass
  type ThisPath = ProcessClassPath

  @volatile
  private[this] var _config = Configuration(None, immutable.IndexedSeq())
  private[this] var _failableAgents: FailableCollection[Agent] = null
  private[this] val clients = mutable.HashSet[CppHttpRemoteApiProcessClient]()

  def stringToPath(o: String) = ProcessClassPath(o)

  def onCppProxyInvalidated(): Unit = {
    for (c ← clients) logger.error(s"CppHttpRemoteApiProcessClient has not been removed: $c")
  }

  @ForCpp
  private def processConfigurationDomElement(element: dom.Element): Unit = {
    changeConfiguration(Configuration.parse(element))
  }

  @ForCpp
  private def replaceWith(other: ProcessClass): Unit = {
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
  private def hasMoreAgents = config.moreAgents.nonEmpty

  @ForCpp
  private def startCppHttpRemoteApiProcessClient(
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
  private def removeCppHttpRemoteApiProcessClient(client: CppHttpRemoteApiProcessClient): Unit = {
    try client.close()
    finally clients -= client
  }

  private[kernel] def agentUris: immutable.Seq[AgentAddress] =
    _config.agents map { _.address }

  private[kernel] def containsAgentUri(agentUri: AgentAddress) = {
    val u = Uri(agentUri.string)
    _config.agents exists { o ⇒ Uri(o.address.string) == u }  // Compare URIs, not strings
  }

  private[kernel] def view[V <: ProcessClassView: ProcessClassView.Companion]: V =
    implicitly[ProcessClassView.Companion[V]] match {
      case ProcessClassOverview ⇒ overview.asInstanceOf[V]
      case ProcessClassDetailed ⇒ detailed.asInstanceOf[V]
    }

  private def detailed = ProcessClassDetailed(
    overview,
    selectionMethod = _config.selectionMethod.name,
    _config.agents map { _.address },
    processes)

  private def overview = ProcessClassOverview(
    path,
    fileBasedState,
    processLimit = processLimit,
    usedProcessCount = usedProcessCount,
    obstacles = obstacles)

  private[kernel] def obstacles: Set[ProcessClassObstacle] = {
    import ProcessClassObstacle._
    val builder = Set.newBuilder[ProcessClassObstacle]
    emptyToNone(fileBasedObstacles) match {
      case Some(o) ⇒
        builder += FileBasedObstacles(o)
      case None ⇒
        if (usedProcessCount >= processLimit) {
          builder += ProcessLimitReached(processLimit)
        }
    }
    builder.result
  }

  private def processLimit: Int = cppProxy.max_processes
  private def usedProcessCount: Int = cppProxy.used_process_count

  private def processes: immutable.Seq[ProcessDetailed] =
    for (process ← domElement / "processes" / "process") yield
      ProcessDetailed(
        jobPath = JobPath(process.getAttribute("job")),
        taskId = TaskId(emptyToNone(process.getAttribute("task")) map { _.toInt } getOrElse 0),
        startedAt =  Instant parse process.getAttribute("running_since"),
        pid = emptyToNone(process.getAttribute("pid")) map { _.toInt },
        agent = emptyToNone(process.getAttribute("remote_scheduler")) map AgentAddress.apply)

  private def domElement: dom.Element = cppProxy.java_dom_element

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

  object Type extends SisterType[ProcessClass, Process_classC] {
    def sister(proxy: Process_classC, context: Sister) = {
      val injector = context.asInstanceOf[HasInjector].injector
      new ProcessClass(
        proxy,
        injector.instance[ProcessClassSubsystem],
        injector.instance[SchedulerThreadCallQueue],
        injector.instance[CppHttpRemoteApiProcessClient.Factory],
        agentConnectRetryDelayLazy = () ⇒ max(1, injector.instance[SpoolerC].settings._remote_scheduler_connect_retry_delay).s)
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
