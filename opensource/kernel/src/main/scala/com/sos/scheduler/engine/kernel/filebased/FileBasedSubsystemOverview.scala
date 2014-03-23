package com.sos.scheduler.engine.kernel.filebased

import com.sos.scheduler.engine.data.filebased.FileBasedType

trait FileBasedSubsystemOverview {
  def fileBasedType: FileBasedType
  def count: Int
  def fileBasedStateCounts: Map[FileBasedState, Int]
}
