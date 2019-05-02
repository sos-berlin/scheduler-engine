package com.sos.scheduler.engine.data.jobchain

import com.sos.scheduler.engine.data.filebased.FileBasedState
import com.sos.scheduler.engine.data.processclass.ProcessClassPath
import com.sos.scheduler.engine.data.queries.QueryableJobChain
import spray.json.DefaultJsonProtocol._

/**
  * @author Joacim Zschimmer
  */
final case class JobChainOverview(
  path: JobChainPath,
  fileBasedState: FileBasedState,
  state: JobChainState,
  jobOrJobChainNodeCount: Int,
  nonBlacklistedOrderCount: Int,
  blacklistedOrderCount: Int = 0,
  hasJobChainNodes: Boolean = false,
  isDistributed: Boolean = false,
  orderLimit: Option[Int] = None,
  title: String = "",
  defaultProcessClassPath: Option[ProcessClassPath] = None,
  fileWatchingProcessClassPath: Option[ProcessClassPath] = None,
  orderIdSpaceName: Option[String] = None,
  obstacles: Set[JobChainObstacle] = Set())
extends QueryableJobChain

object JobChainOverview {
  private implicit val fileBasedStateJsonFormat = FileBasedState.MyJsonFormat
  private implicit val jobChainStateJsonFormat = JobChainState.jsonFormat
  implicit val MyJsonFormat = jsonFormat14(apply)
}
