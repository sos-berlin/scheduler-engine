package com.sos.scheduler.engine.data.monitor

import com.sos.jobscheduler.data.filebased.TypedPath

/**
 * @author Joacim Zschimmer
 */
final case class MonitorPath(string: String) extends TypedPath {
  validate()

  def companion = MonitorPath
}

object MonitorPath extends TypedPath.Companion[MonitorPath]
