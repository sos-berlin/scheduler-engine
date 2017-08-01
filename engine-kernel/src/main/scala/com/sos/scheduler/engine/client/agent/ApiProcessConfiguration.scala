package com.sos.scheduler.engine.client.agent

import com.sos.scheduler.engine.agent.data.commands.{StartApiTask, StartNonApiTask, StartTask}

/**
 * @author Joacim Zschimmer
 */
final case class ApiProcessConfiguration(
  meta: StartTask.Meta,
  hasApi: Boolean,
  javaOptions: String,
  javaClasspath: String,
  logon: Option[StartTask.KeyLogon]) {


  def toUniversalAgentCommand: StartTask = {
    if (hasApi)
      StartApiTask(Some(meta), logon, javaOptions = javaOptions, javaClasspath = javaClasspath)
    else
      StartNonApiTask(Some(meta), logon)
  }
}
