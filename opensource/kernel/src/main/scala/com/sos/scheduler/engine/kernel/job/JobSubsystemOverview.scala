package com.sos.scheduler.engine.kernel.job

import com.sos.scheduler.engine.data.filebased.FileBasedType
import com.sos.scheduler.engine.kernel.filebased.{FileBasedState, FileBasedSubsystemOverview}

final case class JobSubsystemOverview(
  fileBasedType: FileBasedType,
  count: Int,
  fileBasedStateCounts: Map[FileBasedState, Int],
  jobStateCounts: Map[JobState, Int],
  needProcessCount: Int)
extends FileBasedSubsystemOverview