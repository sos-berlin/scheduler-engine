package com.sos.scheduler.engine.agent.task

/**
 * @author Joacim Zschimmer
 */
trait RemoteTask extends AutoCloseable {

  def id: RemoteTaskId

  def start(): Unit

  def kill(): Unit
}
