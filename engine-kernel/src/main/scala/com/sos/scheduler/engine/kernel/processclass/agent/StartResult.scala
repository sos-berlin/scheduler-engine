package com.sos.scheduler.engine.kernel.processclass.agent

import com.sos.scheduler.engine.cplusplus.runtime.annotation.ForCpp
import scala.annotation.meta.getter
import scala.util.Try

/**
  * @author Joacim Zschimmer
  */
@ForCpp
final case class StartResult(
  @(ForCpp @getter) agentUri: String,
  @(ForCpp @getter) result: Try[Unit])
