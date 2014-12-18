package com.sos.scheduler.engine.agent.task

import com.sos.scheduler.engine.agent.task.RemoteTaskId

/**
 * @author Joacim Zschimmer
 */
trait RemoteTask extends AutoCloseable {

  def id: RemoteTaskId

  def kill(): Unit
}
