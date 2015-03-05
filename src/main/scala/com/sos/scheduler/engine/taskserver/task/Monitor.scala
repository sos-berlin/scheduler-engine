package com.sos.scheduler.engine.taskserver.task

import com.sos.scheduler.engine.taskserver.module.Module

/**
 * @author Joacim Zschimmer
 */
final case class Monitor(module: Module, name: String, ordering: Int)

object Monitor {
  val DefaultOrdering = 1
}
