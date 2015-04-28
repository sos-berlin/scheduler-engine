package com.sos.scheduler.engine.kernel.persistence.hibernate

import com.sos.scheduler.engine.data.job.TaskId
import com.sos.scheduler.engine.data.scheduler.VariablePersistentState
import com.sos.scheduler.engine.persistence.entities.{VariableEntity, VariableEntityConverter}
import javax.inject.{Inject, Singleton}
import javax.persistence.{EntityManager, EntityManagerFactory}

/**
 * @author Joacim Zschimmer
 */
@Singleton
final class HibernateVariableStore @Inject private(protected val entityManagerFactory: EntityManagerFactory)
extends AbstractHibernateStore[VariablePersistentState, String, VariableEntity]
with VariableEntityConverter {

  def nextTaskId(implicit em: EntityManager) = TaskId(tryFetch(VariableEntity.JobIdName).get.int)
}
