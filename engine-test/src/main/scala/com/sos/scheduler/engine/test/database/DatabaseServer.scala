package com.sos.scheduler.engine.test.database

/**
 * @author Joacim Zschimmer
 */
trait DatabaseServer extends AutoCloseable {

  def start(): Unit

  def stop(): Unit
}
