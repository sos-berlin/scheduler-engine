package com.sos.scheduler.engine.kernel.filebased

import java.io.File
import org.joda.time.Instant

trait FileBasedDetails extends FileBasedOverview {
  def file: Option[File]
  def fileModificationInstant: Option[Instant]
  def sourceXml: Option[String]
}
