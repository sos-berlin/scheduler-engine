package com.sos.scheduler.engine.client.api

import com.sos.jobscheduler.data.agent.AgentAddress
import com.sos.jobscheduler.data.event.Stamped
import com.sos.scheduler.engine.data.processclass.{ProcessClassPath, ProcessClassView}
import com.sos.scheduler.engine.data.queries.PathQuery
import scala.collection.immutable
import scala.concurrent.Future

/**
  * @author Joacim Zschimmer
  */
trait ProcessClassClient {
  def agentUris: Future[Stamped[Set[AgentAddress]]]

  def processClasses[V <: ProcessClassView: ProcessClassView.Companion](query: PathQuery): Future[Stamped[immutable.Seq[V]]]

  def processClass[V <: ProcessClassView: ProcessClassView.Companion](processClassPath: ProcessClassPath): Future[Stamped[V]]
}
