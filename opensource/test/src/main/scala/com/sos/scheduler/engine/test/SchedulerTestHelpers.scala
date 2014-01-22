package com.sos.scheduler.engine.test

import com.sos.scheduler.engine.common.scalautil.AutoClosing._
import com.sos.scheduler.engine.data.folder.JobPath
import com.sos.scheduler.engine.data.job.TaskClosedEvent
import com.sos.scheduler.engine.test.scala.ScalaSchedulerTest
import com.sos.scheduler.engine.test.scala.SchedulerTestImplicits._

trait SchedulerTestHelpers {
  this: ScalaSchedulerTest =>

  final def runJobAndWaitForEnd(jobPath: JobPath) {
    autoClosing(controller.newEventPipe()) { eventPipe =>
      scheduler executeXml <start_job job={jobPath.string}/>
      eventPipe.nextWithCondition[TaskClosedEvent] { _.jobPath == jobPath }
    }
  }
}
