package com.sos.scheduler.engine.kernel.persistence

import com.sos.scheduler.engine.data.folder.JobPath
import com.sos.scheduler.engine.data.job.JobPersistent
import com.sos.scheduler.engine.data.scheduler.{ClusterMemberId, SchedulerId}
import com.sos.scheduler.engine.persistence.entities.{JobEntityConverter, JobEntity}
import javax.inject.{Inject, Singleton}
import javax.persistence.{EntityManager, EntityManagerFactory}

@Singleton
final class JobStore @Inject()(
    protected val schedulerId: SchedulerId,
    protected val clusterMemberId: ClusterMemberId,
    protected val entityManagerFactory: EntityManagerFactory)
extends AbstractObjectJPAStore[JobPersistent, JobPath, JobEntity]
with JobEntityConverter {

  override def store(o: JobPersistent)(implicit em: EntityManager) {
    if (o.isDefault)
      delete(o.jobPath)   // Den normalen Zustand speichern wir nicht
    else
      super.store(o)
  }
}
