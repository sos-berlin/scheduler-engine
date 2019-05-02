package com.sos.scheduler.engine.data.queries

import com.sos.scheduler.engine.data.jobchain.{JobChainPath, JobChainState}
import org.jetbrains.annotations.TestOnly

/**
  * @author Joacim Zschimmer
  */
trait QueryableJobChain {
  def path: JobChainPath
  def isDistributed: Boolean
  def state: JobChainState
}

object QueryableJobChain {
  @TestOnly
  final case class ForTest(
    path: JobChainPath,
    isDistributed: Boolean = false,
    state: JobChainState = JobChainState.running)
  extends QueryableJobChain
}
