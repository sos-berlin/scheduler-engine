package com.sos.scheduler.engine.client.api

import com.sos.jobscheduler.data.event.Snapshot
import com.sos.jobscheduler.data.filebased.TypedPath
import com.sos.scheduler.engine.data.filebased.FileBasedView
import com.sos.scheduler.engine.data.queries.PathQuery
import scala.collection.immutable
import scala.concurrent.Future

/**
  * @author Joacim Zschimmer
  */
trait FileBasedClient {

  def fileBased[P <: TypedPath, V <: FileBasedView: FileBasedView.Companion](path: P): Future[Snapshot[V]]

  def fileBaseds[P <: TypedPath: TypedPath.Companion, V <: FileBasedView: FileBasedView.Companion](query: PathQuery): Future[Snapshot[immutable.Seq[V]]]

  def anyTypeFileBaseds[V <: FileBasedView: FileBasedView.Companion](query: PathQuery): Future[Snapshot[immutable.Seq[V]]]
}
