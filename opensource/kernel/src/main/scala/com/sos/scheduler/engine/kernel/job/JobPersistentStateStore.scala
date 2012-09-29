package com.sos.scheduler.engine.kernel.job

import com.sos.scheduler.engine.data.folder.JobPath
import com.sos.scheduler.engine.data.job.JobPersistentState
import com.sos.scheduler.engine.data.scheduler.{ClusterMemberId, SchedulerId}
import com.sos.scheduler.engine.persistence.entities.JobEntity
import com.sos.scheduler.engine.persistence.entities.JobEntity.PrimaryKey
import javax.inject.{Inject, Singleton}
import javax.persistence.{EntityManager, EntityManagerFactory}

@Singleton
class JobPersistentStateStore @Inject()(schedulerId: SchedulerId, clusterMemberId: ClusterMemberId,
    protected val entityManagerFactory: EntityManagerFactory)
extends AbstractObjectJPAStore[JobEntity, JobPersistentState, JobPath] {

  protected final def toEntityKey(jobPath: JobPath) = PrimaryKey(schedulerId, clusterMemberId, jobPath)

  protected final def toEntity(o: JobPersistentState) = JobEntity(schedulerId, clusterMemberId, o)

  final def toObject(e: JobEntity) = e.toObject

  override final def store(o: JobPersistentState)(implicit em: EntityManager) {
    if (o.isDefault)
      delete(o.jobPath)
    else
      super.store(o)
  }
}
