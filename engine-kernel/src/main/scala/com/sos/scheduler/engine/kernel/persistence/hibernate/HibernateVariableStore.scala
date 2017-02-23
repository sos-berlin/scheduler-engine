package com.sos.scheduler.engine.kernel.persistence.hibernate

import com.sos.jobscheduler.data.job.TaskId
import com.sos.scheduler.engine.data.scheduler.VariablePersistentState
import com.sos.scheduler.engine.persistence.entities.{VariableEntity, VariableEntityConverter}
import javax.inject.{Inject, Singleton}
import javax.persistence.{EntityManager, EntityManagerFactory}

/**
 * @author Joacim Zschimmer
 */
@Singleton
final class HibernateVariableStore @Inject private(protected val entityManagerFactory: EntityManagerFactory)
extends AbstractHibernateStore[VariablePersistentState, VariableEntity]
with VariableEntityConverter {

  def nextTaskId(implicit em: EntityManager) = TaskId(fetch(VariableEntity.JobIdName).int)
}
