package com.sos.scheduler.engine.taskserver.task

import com.sos.scheduler.engine.minicom.types.IDispatch
import org.scalatest.FreeSpec
import org.scalatest.Matchers._

/**
 * @author Joacim Zschimmer
 */
final class NamedObjectsTest extends FreeSpec {

  "spoolerLog" in {
    val spoolerLog = new IDispatch {}
    val spoolerTask = new IDispatch {}
    val spoolerJob = new IDispatch {}
    val spooler = new IDispatch {}
    val namedObjects = NamedObjects(List(
      "spooler_log" → spoolerLog,
      "spooler_task" → spoolerTask,
      "spooler_job" → spoolerJob,
      "spooler" → spooler))
    namedObjects.spoolerLog shouldEqual Some(spoolerLog)
    namedObjects.toMap.size shouldEqual 4
  }

  "invalid name" in {
    intercept[IllegalArgumentException] { NamedObjects(List("invalid" → new IDispatch {})) }
  }
}
