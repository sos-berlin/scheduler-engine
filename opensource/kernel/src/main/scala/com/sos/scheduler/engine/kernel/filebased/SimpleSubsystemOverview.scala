package com.sos.scheduler.engine.kernel.filebased

import com.sos.scheduler.engine.data.filebased.FileBasedType

final case class SimpleSubsystemOverview(
  fileBasedType: FileBasedType,
  count: Int,
  fileBasedStateCounts: Map[FileBasedState, Int])
extends FileBasedSubsystemOverview
