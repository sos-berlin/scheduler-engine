package com.sos.scheduler.engine.kernel.job

import com.sos.scheduler.engine.data.filebased.{FileBasedState, FileBasedType}
import com.sos.scheduler.engine.kernel.filebased.FileBasedSubsystemOverview

final case class JobSubsystemOverview(
  fileBasedType: FileBasedType,
  count: Int,
  fileBasedStateCounts: Map[FileBasedState, Int],
  jobStateCounts: Map[JobState, Int],
  needProcessCount: Int)
extends FileBasedSubsystemOverview