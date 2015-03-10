package com.sos.scheduler.engine.taskserver.module

import com.sos.scheduler.engine.common.scalautil.Collections.implicits.RichPairTraversable
import com.sos.scheduler.engine.minicom.idispatch.Invocable
import com.sos.scheduler.engine.taskserver.module.NamedInvocables._
import com.sos.scheduler.engine.taskserver.spoolerapi.{SpoolerLog, SpoolerTask}

/**
 * @author Joacim Zschimmer
 */
final class NamedInvocables private(val toMap: Map[String, Invocable]) {
  lazy val spoolerLog: SpoolerLog = toMap(SpoolerLogName).asInstanceOf[SpoolerLog]
  lazy val spoolerTask: SpoolerTask = toMap(SpoolerTaskName).asInstanceOf[SpoolerTask]
}

object NamedInvocables {
  val SpoolerLogName = "spooler_log"
  val SpoolerTaskName = "spooler_task"
  val SpoolerJobName = "spooler_job"
  val SpoolerName = "spooler"
  val AllNames = Set(SpoolerLogName, SpoolerTaskName, SpoolerJobName, SpoolerName)

  def apply(kv: Iterable[(String, Invocable)]): NamedInvocables = {
    val invalidNames = (kv map { _._1 }).toSet -- AllNames
    require(invalidNames.isEmpty, s"Invalid object names: $invalidNames")
    new NamedInvocables(kv.uniqueToMap)
  }
}
