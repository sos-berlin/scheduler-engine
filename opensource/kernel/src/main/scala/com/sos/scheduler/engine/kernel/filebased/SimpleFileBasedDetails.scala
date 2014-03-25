package com.sos.scheduler.engine.kernel.filebased

import com.sos.scheduler.engine.data.filebased.{FileBasedState, FileBasedDetails, TypedPath}
import java.io.File
import org.joda.time.Instant

final case class SimpleFileBasedDetails(
  path: TypedPath,
  fileBasedState: FileBasedState,
  file: Option[File],
  fileModificationInstant: Option[Instant],
  sourceXml: Option[String])
extends FileBasedDetails
