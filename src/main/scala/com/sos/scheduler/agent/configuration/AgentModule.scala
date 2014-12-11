package com.sos.scheduler.agent.configuration

import akka.actor.ActorSystem
import com.google.common.io.Closer
import com.google.inject.{Provider, TypeLiteral}
import com.sos.scheduler.agent.CommandExecutor
import com.sos.scheduler.agent.command.{Command, Response}
import com.sos.scheduler.engine.common.guice.ScalaAbstractModule
import com.sos.scheduler.engine.common.scalautil.SideEffect._
import com.sos.scheduler.engine.common.scalautil.{HasCloser, Logger}

/**
 * @author Joacim Zschimmer
 */
final class AgentModule(agentConfiguration: AgentConfiguration) extends ScalaAbstractModule with HasCloser {

  protected def configure() = {
    bindInstance[Closer](closer)
    bindInstance[AgentConfiguration](agentConfiguration)
    bind(new TypeLiteral[Command ⇒ Response]() {}) toProvider new Provider[Command ⇒ Response] { def get = new CommandExecutor }
    provide[ActorSystem] {
      ActorSystem("JobScheduler-Agent") sideEffect { o ⇒ onClose { o.shutdown() } }
    }
  }
}

private object AgentModule {
  private val logger = Logger(getClass)
}
