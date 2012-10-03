package com.sos.scheduler.engine.data.job

import com.sos.scheduler.engine.cplusplus.runtime.annotation.ForCpp
import com.sos.scheduler.engine.data.folder.JobPath
import org.joda.time.DateTime
import scala.annotation.target.getter

@ForCpp
case class JobPersistent(
    jobPath: JobPath,
    @(ForCpp @getter) isPermanentlyStopped: Boolean,
    nextStartTimeOption: Option[DateTime]) {

  def isDefault = nextStartTimeOption.isEmpty && !isPermanentlyStopped
}
