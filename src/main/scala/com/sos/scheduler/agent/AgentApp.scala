package com.sos.scheduler.agent

import com.google.common.io.Closer
import com.google.inject.Guice
import com.sos.scheduler.agent.configuration.{AgentConfiguration, AgentModule}
import com.sos.scheduler.engine.common.guice.GuiceImplicits._

/**
 * @author Joacim Zschimmer
 */
final class AgentApp(conf: AgentConfiguration) extends AutoCloseable {

  private val injector = Guice.createInjector(new AgentModule(conf))

  def start() = injector.apply[Agent].start()

  def close(): Unit = {
    injector.apply[Closer].close()
  }
}
