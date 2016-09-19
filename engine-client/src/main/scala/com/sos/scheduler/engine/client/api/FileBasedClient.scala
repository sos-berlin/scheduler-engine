package com.sos.scheduler.engine.client.api

import com.sos.scheduler.engine.data.event.Snapshot
import com.sos.scheduler.engine.data.filebased.{FileBasedDetailed, TypedPath}
import scala.concurrent.Future

/**
  * @author Joacim Zschimmer
  */
trait FileBasedClient {

  def fileBasedDetailed[P <: TypedPath: TypedPath.Companion](path: P): Future[Snapshot[FileBasedDetailed]]
}
