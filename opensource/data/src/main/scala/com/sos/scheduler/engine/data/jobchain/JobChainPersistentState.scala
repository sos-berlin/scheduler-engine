package com.sos.scheduler.engine.data.jobchain

import com.sos.scheduler.engine.data.base.{HasKey, HasIsDefault}
import com.sos.scheduler.engine.data.jobchain.JobChainPath

final case class JobChainPersistentState(jobChainPath: JobChainPath, isStopped: Boolean)
extends HasKey[JobChainPath] with HasIsDefault {

  def key = jobChainPath

  def isDefault = !isStopped
}
