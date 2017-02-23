package com.sos.scheduler.engine.data.filebased

import com.sos.jobscheduler.data.filebased.TypedPath

/**
  * @author Joacim Zschimmer
  */
trait HasPath {
  def path: TypedPath
}
