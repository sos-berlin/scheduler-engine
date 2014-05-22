package com.sos.scheduler.engine.kernel.filebased

import com.sos.scheduler.engine.data.filebased.{FileBasedState, FileBasedOverview, TypedPath}

final case class SimpleFileBasedOverview(
  path: TypedPath,
  fileBasedState: FileBasedState)
extends FileBasedOverview
