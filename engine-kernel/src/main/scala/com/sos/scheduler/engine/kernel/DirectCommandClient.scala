package com.sos.scheduler.engine.kernel

import com.sos.scheduler.engine.client.api.CommandClient
import scala.concurrent.ExecutionContext

/**
  * @author Joacim Zschimmer
  */
trait DirectCommandClient
extends CommandClient with CommandClient.Defaults {

  protected def scheduler: Scheduler
  protected implicit def executionContext: ExecutionContext

  /**
    * Like uncheckedExecuteXml, but fails when response contains an ERROR element.
    *
    * @return XML response
    */
  final def executeXml(string: String) = scheduler.executeXmlFuture(string)


  /**
    * @return XML response
    */
  final def uncheckedExecuteXml(string: String) = scheduler.uncheckedExecuteXmlFuture(string)
}

object DirectCommandClient {
  private val ReadOnlyCommandPrefixes = List("<show_", "show_", "<s ", "<s/>", "s ")

  def commandIsReadOnly(command: String) = ReadOnlyCommandPrefixes exists s"$command ".startsWith
}
