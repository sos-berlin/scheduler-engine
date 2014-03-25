package com.sos.scheduler.engine.kernel.order.jobchain

import com.sos.scheduler.engine.data.jobchain.JobChainPath
import com.sos.scheduler.engine.kernel.filebased.{FileBasedDetails, FileBasedState}
import java.io.File
import org.joda.time.Instant
import scala.collection.immutable

final case class JobChainDetails(
  path: JobChainPath,
  fileBasedState: FileBasedState,
  file: Option[File],
  fileModificationInstant: Option[Instant],
  sourceXml: Option[String],
  nodes: immutable.Seq[NodeOverview]
)
extends FileBasedDetails
