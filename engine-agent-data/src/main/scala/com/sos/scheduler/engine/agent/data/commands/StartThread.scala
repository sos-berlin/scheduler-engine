package com.sos.scheduler.engine.agent.data.commands

/**
 * @author Joacim Zschimmer
 */
final case class StartThread(controllerAddressOption: Option[String])
extends StartProcess
