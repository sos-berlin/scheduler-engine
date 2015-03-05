package com.sos.scheduler.engine.taskserver.module

import com.sos.scheduler.engine.common.scalautil.Collections.implicits.RichPairTraversable
import com.sos.scheduler.engine.minicom.idispatch.IDispatchable
import com.sos.scheduler.engine.taskserver.module.NamedObjects._
import com.sos.scheduler.engine.taskserver.spoolerapi.{SpoolerLog, SpoolerTask}

/**
 * @author Joacim Zschimmer
 */
final class NamedObjects private(val toMap: Map[String, IDispatchable]) {
  lazy val spoolerLog: SpoolerLog = toMap(SpoolerLogName).asInstanceOf[SpoolerLog]
  lazy val spoolerTask: SpoolerTask = toMap(SpoolerTaskName).asInstanceOf[SpoolerTask]
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
    new NamedObjects(kv.uniqueToMap)
  }
}
