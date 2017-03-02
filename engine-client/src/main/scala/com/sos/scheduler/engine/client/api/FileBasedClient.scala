package com.sos.scheduler.engine.client.api

import com.sos.jobscheduler.data.event.Stamped
import com.sos.jobscheduler.data.filebased.TypedPath
import com.sos.scheduler.engine.data.filebased.FileBasedView
import com.sos.scheduler.engine.data.queries.PathQuery
import scala.collection.immutable
import scala.concurrent.Future

/**
  * @author Joacim Zschimmer
  */
trait FileBasedClient {

  def fileBased[P <: TypedPath, V <: FileBasedView: FileBasedView.Companion](path: P): Future[Stamped[V]]

  def fileBaseds[P <: TypedPath: TypedPath.Companion, V <: FileBasedView: FileBasedView.Companion](query: PathQuery): Future[Stamped[immutable.Seq[V]]]

  def anyTypeFileBaseds[V <: FileBasedView: FileBasedView.Companion](query: PathQuery): Future[Stamped[immutable.Seq[V]]]
}
