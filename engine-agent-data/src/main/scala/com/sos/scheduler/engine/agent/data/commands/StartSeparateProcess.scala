package com.sos.scheduler.engine.agent.data.commands

/**
 * @author Joacim Zschimmer
 */
final case class StartSeparateProcess(controllerAddress: String, javaOptions: String, javaClasspath: String)
extends StartProcess
