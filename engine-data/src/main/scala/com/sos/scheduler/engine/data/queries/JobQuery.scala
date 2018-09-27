package com.sos.scheduler.engine.data.queries

import com.sos.scheduler.engine.data.folder.FolderPath
import com.sos.scheduler.engine.data.job.JobState
import scala.language.implicitConversions

/**
  * @author Joacim Zschimmer
  */
final case class JobQuery(
  pathQuery: PathQuery = PathQuery.All,
  isInState: JobState ⇒ Boolean = _ ⇒ true)
{
  def toUriPath = pathQuery.toUriPath
}

object JobQuery {
  val All = JobQuery()

  implicit def fromPathQuery(query: PathQuery): JobQuery =
    JobQuery(query)

  implicit def fromPathQuery(folderPath: FolderPath): JobQuery =
    JobQuery(folderPath)
}
