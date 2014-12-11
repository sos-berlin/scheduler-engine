package com.sos.scheduler.taskserver

import com.sos.scheduler.engine.common.scalautil.Logger
import com.sos.scheduler.taskserver.configuration.StartConfiguration

/**
 * @author Joacim Zschimmer
 */
final class Main {
}

private object Main {
  private val logger = Logger(getClass)

  def main(arg: Array[String]): Unit = {
    // <task_process> von stdin lesen
    val startConfiguration: StartConfiguration = ???
    TaskServer.run(startConfiguration)
  }
}
