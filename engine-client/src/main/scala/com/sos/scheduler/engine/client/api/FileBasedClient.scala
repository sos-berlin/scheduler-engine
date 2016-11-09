package com.sos.scheduler.engine.client.api

import com.sos.scheduler.engine.data.event.Snapshot
import com.sos.scheduler.engine.data.filebased.{FileBasedDetailed, TypedPath}
import com.sos.scheduler.engine.data.queries.PathQuery
import scala.collection.immutable
import scala.concurrent.Future

/**
  * @author Joacim Zschimmer
  */
trait FileBasedClient {

  def fileBasedDetailed[P <: TypedPath](path: P): Future[Snapshot[FileBasedDetailed]]

  def fileBasedDetaileds[P <: TypedPath: TypedPath.Companion](query: PathQuery): Future[Snapshot[immutable.Seq[FileBasedDetailed]]]
}
