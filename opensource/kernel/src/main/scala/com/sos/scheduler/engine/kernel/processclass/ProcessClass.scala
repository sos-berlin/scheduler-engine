package com.sos.scheduler.engine.kernel.processclass

import com.sos.scheduler.engine.client.agent.ApiProcessConfiguration
import com.sos.scheduler.engine.common.inject.GuiceImplicits._
import com.sos.scheduler.engine.common.time.ScalaJoda._
import com.sos.scheduler.engine.cplusplus.runtime.annotation.ForCpp
import com.sos.scheduler.engine.cplusplus.runtime.{Sister, SisterType}
import com.sos.scheduler.engine.data.filebased.FileBasedType
import com.sos.scheduler.engine.data.processclass.ProcessClassPath
import com.sos.scheduler.engine.kernel.async.SchedulerThreadCallQueue
import com.sos.scheduler.engine.kernel.cppproxy.{Api_process_configurationC, Process_classC}
import com.sos.scheduler.engine.kernel.filebased.FileBased
import com.sos.scheduler.engine.kernel.processclass.ProcessClass._
import com.sos.scheduler.engine.kernel.processclass.agent.{Agent, CppHttpRemoteApiProcessClient}
import com.sos.scheduler.engine.kernel.processclass.common.FailableCollection
import com.sos.scheduler.engine.kernel.scheduler.HasInjector
import org.scalactic.Requirements._
import org.w3c.dom
import scala.collection.immutable

@ForCpp
final class ProcessClass private(
  protected[this] val cppProxy: Process_classC,
  protected val subsystem: ProcessClassSubsystem,
  callQueue: SchedulerThreadCallQueue,
  newCppHttpRemoteApiProcessClient: CppHttpRemoteApiProcessClient.Factory)
extends FileBased {

  type Path = ProcessClassPath

  private[this] var _config = Configuration(None, immutable.IndexedSeq())
  private[this] var _failableAgents: FailableCollection[Agent] = null

  def stringToPath(o: String) = ProcessClassPath(o)

  def fileBasedType = FileBasedType.processClass

  def onCppProxyInvalidated() = {}

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
      // TODO Noch laufende Anwendungen von failableAgents abbrechen - die laufen solange, bis ein Agent erreichbar ist.
      _failableAgents = new FailableCollection(_config.agents, InaccessibleAgentDelay)
    }
  }

  @ForCpp
  def hasMoreAgents = config.moreAgents.nonEmpty

  @ForCpp
  def newCppHttpRemoteApiProcessClient(conf: Api_process_configurationC): CppHttpRemoteApiProcessClient =
    newCppHttpRemoteApiProcessClient(failableAgents, apiProcessConfiguration(conf))

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
  val InaccessibleAgentDelay = 30.s

  final class Type extends SisterType[ProcessClass, Process_classC] {
    def sister(proxy: Process_classC, context: Sister) = {
      val injector = context.asInstanceOf[HasInjector].injector
      new ProcessClass(
        proxy,
        injector.apply[ProcessClassSubsystem],
        injector.apply[SchedulerThreadCallQueue],
        injector.apply[CppHttpRemoteApiProcessClient.Factory])
    }
  }

  def apiProcessConfiguration(c: Api_process_configurationC) = new ApiProcessConfiguration(
    hasApi = c._has_api,
    javaOptions = c._java_options,
    javaClasspath = c._java_classpath)
}
