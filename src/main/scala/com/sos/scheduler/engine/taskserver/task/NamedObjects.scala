package com.sos.scheduler.engine.taskserver.task

import com.sos.scheduler.engine.minicom.idispatch.IDispatchable
import com.sos.scheduler.engine.taskserver.spoolerapi.SpoolerLog
import com.sos.scheduler.engine.taskserver.task.NamedObjects._

/**
 * @author Joacim Zschimmer
 */
final class NamedObjects private(val toMap: Map[String, IDispatchable]) {
  def spoolerLog: SpoolerLog = toMap(SpoolerLogName).asInstanceOf[SpoolerLog]  // instanceOf???
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
