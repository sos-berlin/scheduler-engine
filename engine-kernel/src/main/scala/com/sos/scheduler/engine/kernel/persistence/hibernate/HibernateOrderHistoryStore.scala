package com.sos.scheduler.engine.kernel.persistence.hibernate

import com.sos.scheduler.engine.data.order.{OrderHistoryId, OrderHistoryPersistentState}
import com.sos.scheduler.engine.data.scheduler.SchedulerId
import com.sos.scheduler.engine.persistence.entities.{OrderHistoryEntity, OrderHistoryEntityConverter}
import javax.inject.Inject
import javax.persistence.EntityManagerFactory

final class HibernateOrderHistoryStore @Inject private(
   protected val schedulerId: SchedulerId,
   protected val entityManagerFactory: EntityManagerFactory)
extends AbstractHibernateStore[OrderHistoryPersistentState, OrderHistoryId, OrderHistoryEntity]
with OrderHistoryEntityConverter
