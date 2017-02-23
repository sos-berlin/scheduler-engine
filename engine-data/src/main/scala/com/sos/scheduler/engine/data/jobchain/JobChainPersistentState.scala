package com.sos.scheduler.engine.data.jobchain

import com.sos.jobscheduler.base.generic.HasIsDefault
import com.sos.jobscheduler.base.utils.HasKey

final case class JobChainPersistentState(jobChainPath: JobChainPath, isStopped: Boolean)
extends HasKey with HasIsDefault {

  type Key = JobChainPath

  def key = jobChainPath

  def isDefault = !isStopped
}
