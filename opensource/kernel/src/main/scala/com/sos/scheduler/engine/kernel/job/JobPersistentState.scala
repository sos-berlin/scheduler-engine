package com.sos.scheduler.engine.kernel.job

import com.sos.scheduler.engine.cplusplus.runtime.annotation.ForCpp
import com.sos.scheduler.engine.data.folder.JobPath
import org.joda.time.DateTime
import scala.annotation.target.getter

@ForCpp
case class JobPersistentState(
    jobPath: JobPath,
    nextStartTimeOption: Option[DateTime],
    @(ForCpp @getter) isPermanentlyStopped: Boolean) {

  def isDefault = nextStartTimeOption.isEmpty && !isPermanentlyStopped
}
