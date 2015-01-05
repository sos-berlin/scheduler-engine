package com.sos.scheduler.engine.agent

import com.google.common.io.Closer
import com.google.inject.Guice
import com.sos.scheduler.engine.agent.configuration.AgentConfiguration
import com.sos.scheduler.engine.agent.configuration.inject.AgentModule
import com.sos.scheduler.engine.common.guice.GuiceImplicits._

/**
 * @author Joacim Zschimmer
 */
final class Main(conf: AgentConfiguration) extends AutoCloseable {

  private val injector = Guice.createInjector(new AgentModule(conf))

  def start() = injector.apply[AgentStarter].start()

  def close() = injector.apply[Closer].close()
}

object Main {
  def main(args: Seq[String]): Unit = {
    val httpPort: Int = ???
    val conf = new AgentConfiguration(httpPort = httpPort)
    val m = new Main(conf)
    m.start()
    ??? // Warten, bis Agent per Kommando beendet wird
  }
}
