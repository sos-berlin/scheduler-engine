package com.sos.scheduler.engine.taskserver.task

import com.sos.scheduler.engine.minicom.idispatch.IDispatchable
import org.scalatest.FreeSpec
import org.scalatest.Matchers._

/**
 * @author Joacim Zschimmer
 */
final class NamedObjectsTest extends FreeSpec {

  "spoolerLog" in {
    val spoolerLog = new IDispatchable {}
    val spoolerTask = new IDispatchable {}
    val spoolerJob = new IDispatchable {}
    val spooler = new IDispatchable {}
    val namedObjects = NamedObjects(List(
      "spooler_log" → spoolerLog,
      "spooler_task" → spoolerTask,
      "spooler_job" → spoolerJob,
      "spooler" → spooler))
    namedObjects.spoolerLog shouldEqual Some(spoolerLog)
    namedObjects.toMap.size shouldEqual 4
  }

  "invalid name" in {
    intercept[IllegalArgumentException] { NamedObjects(List("invalid" → new IDispatchable {})) }
  }
}
