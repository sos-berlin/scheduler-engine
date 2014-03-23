package com.sos.scheduler.engine.kernel.filebased

import com.sos.scheduler.engine.data.filebased.TypedPath
import java.io.File
import org.joda.time.Instant

final case class SimpleFileBasedDetails(
  path: TypedPath,
  fileBasedState: FileBasedState,
  file: Option[File],
  fileModificationInstant: Option[Instant],
  sourceXml: String)
extends FileBasedDetails
