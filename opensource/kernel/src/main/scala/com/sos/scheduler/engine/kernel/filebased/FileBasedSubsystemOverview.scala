package com.sos.scheduler.engine.kernel.filebased

import com.sos.scheduler.engine.data.filebased.{FileBasedState, FileBasedType}

trait FileBasedSubsystemOverview {
  def fileBasedType: FileBasedType
  def count: Int
  def fileBasedStateCounts: Map[FileBasedState, Int]
}
