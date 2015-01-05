package com.sos.scheduler.engine.agent.configuration.inject

import akka.actor.ActorSystem
import com.google.common.io.Closer
import com.google.inject.Provides
import com.sos.scheduler.engine.agent.command.{AgentCommandExecutor, CommandExecutor}
import com.sos.scheduler.engine.agent.configuration.AgentConfiguration
import com.sos.scheduler.engine.agent.task.{RemoteTask, RemoteTaskFactory, RemoteTaskId}
import com.sos.scheduler.engine.common.guice.ScalaAbstractModule
import com.sos.scheduler.engine.common.scalautil.HasCloser
import com.sos.scheduler.engine.common.scalautil.SideEffect._
import com.sos.scheduler.engine.taskserver.configuration.StartConfiguration
import javax.inject.Singleton

/**
 * @author Joacim Zschimmer
 */
final class AgentModule(agentConfiguration: AgentConfiguration) extends ScalaAbstractModule with HasCloser {

  protected def configure() = {
    bindInstance[Closer](closer)
    bindInstance[AgentConfiguration](agentConfiguration)
    provide[ActorSystem] {
      ActorSystem("JobScheduler-Agent") sideEffect { o ⇒ onClose { o.shutdown() } }
    }
    bindClass[CommandExecutor] to classOf[AgentCommandExecutor]
  }

  @Provides @Singleton
  private def newRemoteTask: StartConfiguration ⇒ RemoteTask = RemoteTaskFactory.apply

  @Provides @Singleton
  private def newRemoteTaskId: () ⇒ RemoteTaskId = RemoteTaskId.newGenerator().next
}
