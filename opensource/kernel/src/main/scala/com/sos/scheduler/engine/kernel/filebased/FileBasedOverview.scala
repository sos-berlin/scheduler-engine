package com.sos.scheduler.engine.kernel.filebased

import com.sos.scheduler.engine.data.filebased.TypedPath

trait FileBasedOverview {
  def path: TypedPath
  def fileBasedState: FileBasedState
}

