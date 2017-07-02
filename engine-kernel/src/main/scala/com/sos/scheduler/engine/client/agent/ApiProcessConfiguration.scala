package com.sos.scheduler.engine.client.agent

import com.sos.jobscheduler.agent.data.commands.AgentCommand.{StartApiTask, StartNonApiTask, StartTask}

/**
 * @author Joacim Zschimmer
 */
final case class ApiProcessConfiguration(
  meta: StartTask.Meta,
  hasApi: Boolean,
  javaOptions: String,
  javaClasspath: String) {


  def toUniversalAgentCommand: StartTask = {
    if (hasApi)
      StartApiTask(Some(meta), javaOptions = javaOptions, javaClasspath = javaClasspath)
    else
      StartNonApiTask(Some(meta))
  }
}
