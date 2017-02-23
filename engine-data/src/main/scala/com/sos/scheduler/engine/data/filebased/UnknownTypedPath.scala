package com.sos.scheduler.engine.data.filebased

import com.sos.jobscheduler.data.filebased.TypedPath

/**
  * @author Joacim Zschimmer
  */
final case class UnknownTypedPath(string: String) extends TypedPath {
  validate()

  override def companion = UnknownTypedPath
}

object UnknownTypedPath extends TypedPath.Companion[UnknownTypedPath]{

  override val camelName = "Unknown"

  override def isEmptyAllowed = true
  override def isSingleSlashAllowed = true
  override def isCommaAllowed = true
}
