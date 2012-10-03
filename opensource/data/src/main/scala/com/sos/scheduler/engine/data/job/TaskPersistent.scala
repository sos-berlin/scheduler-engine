package com.sos.scheduler.engine.data.job

import com.sos.scheduler.engine.cplusplus.runtime.annotation.ForCpp
import com.sos.scheduler.engine.data.folder.JobPath
import org.joda.time.DateTime
import scala.annotation.target.getter

@ForCpp
case class TaskPersistent(
  @(ForCpp @getter) taskId: TaskId,
                    jobPath: JobPath,
  @(ForCpp @getter) enqueueTime: DateTime,
                    startTimeOption: Option[DateTime],
  @(ForCpp @getter) parametersXml: String,
  @(ForCpp @getter) xml: String) {

  @ForCpp def startTimeMillis: Long = startTimeOption map { _.getMillis } getOrElse 0
}
