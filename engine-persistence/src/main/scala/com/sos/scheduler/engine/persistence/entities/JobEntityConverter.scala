package com.sos.scheduler.engine.persistence.entities

import com.sos.jobscheduler.data.scheduler.SchedulerId
import com.sos.scheduler.engine.data.job.{JobPath, JobPersistentState}
import com.sos.scheduler.engine.data.scheduler.ClusterMemberId
import com.sos.scheduler.engine.persistence.SchedulerDatabases._
import com.sos.scheduler.engine.persistence.entity.ObjectEntityConverter

trait JobEntityConverter extends ObjectEntityConverter[JobPersistentState, JobPath, JobEntity] {
  protected val schedulerId: SchedulerId
  protected val clusterMemberId: ClusterMemberId

  final def toObject(e: JobEntity) =
    JobPersistentState(
      jobPath = JobPath("/"+ e.jobPath),
      isPermanentlyStopped = e.isStopped,
      nextStartTimeOption = Option(e.nextStartTime) map databaseToInstant)

  final def toEntity(o: JobPersistentState) = {
    val k = toEntityKey(o.jobPath)
    val e = new JobEntity
    e.schedulerId = k.schedulerId
    e.clusterMemberId = k.clusterMemberId
    e.jobPath = k.jobPath
    e.isStopped = o.isPermanentlyStopped
    e.nextStartTime = (o.nextStartTimeOption map instantToDatabase).orNull
    e
  }

  final def toEntityKey(jobPath: JobPath) =
    JobEntityKey(
      schedulerIdToDatabase(schedulerId),
      if (clusterMemberId.isEmpty) "-" else clusterMemberId.string,
      jobPath.withoutStartingSlash)
}
