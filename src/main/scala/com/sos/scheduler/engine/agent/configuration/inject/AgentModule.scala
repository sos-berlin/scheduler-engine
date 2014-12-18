package com.sos.scheduler.engine.agent.configuration.inject

import akka.actor.ActorSystem
import com.google.common.io.Closer
import com.google.inject.Provides
import com.sos.scheduler.engine.agent.command.{CommandExecutor, AgentCommandExecutor}
import com.sos.scheduler.engine.agent.commands.{Command, Response}
import com.sos.scheduler.engine.agent.configuration.AgentConfiguration
import com.sos.scheduler.engine.common.guice.ScalaAbstractModule
import com.sos.scheduler.engine.common.scalautil.HasCloser
import com.sos.scheduler.engine.common.scalautil.SideEffect._

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
  }

  @Provides
  private def commandExecutor(o: AgentCommandExecutor): CommandExecutor = o
}
