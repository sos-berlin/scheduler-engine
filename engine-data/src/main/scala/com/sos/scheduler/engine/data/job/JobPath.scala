package com.sos.scheduler.engine.data.job

import com.sos.jobscheduler.data.filebased.TypedPath

final case class JobPath(string: String)
extends TypedPath {

  validate()

  def companion = JobPath
}

object JobPath extends TypedPath.Companion[JobPath]
