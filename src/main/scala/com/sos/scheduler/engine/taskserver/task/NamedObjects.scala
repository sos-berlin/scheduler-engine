package com.sos.scheduler.engine.taskserver.task

import com.sos.scheduler.engine.minicom.types.IDispatchable
import com.sos.scheduler.engine.taskserver.task.NamedObjects._

/**
 * @author Joacim Zschimmer
 */
final class NamedObjects private(val toMap: Map[String, IDispatchable]) {
  def spoolerLog: Option[IDispatchable] = toMap.get(SpoolerLogName)
}

object NamedObjects {
  private val SpoolerLogName = "spooler_log"
  private val SpoolerTaskName = "spooler_task"
  private val SpoolerJobName = "spooler_job"
  private val SpoolerName = "spooler"
  private val AllNames = Set(SpoolerLogName, SpoolerTaskName, SpoolerJobName, SpoolerName)

  def apply(kv: Iterable[(String, IDispatchable)]): NamedObjects = {
    val invalidNames = (kv map { _._1 }).toSet -- AllNames
    require(invalidNames.isEmpty, s"Invalid object names: $invalidNames")
    new NamedObjects(kv.toMap)
  }
}
