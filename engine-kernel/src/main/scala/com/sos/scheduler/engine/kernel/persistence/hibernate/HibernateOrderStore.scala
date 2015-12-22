package com.sos.scheduler.engine.kernel.persistence.hibernate

import com.sos.scheduler.engine.data.order.{OrderPersistentState, OrderKey}
import com.sos.scheduler.engine.data.scheduler.SchedulerId
import com.sos.scheduler.engine.persistence.entities.{OrderEntityConverter, OrderEntity}
import javax.inject.Inject
import javax.persistence.EntityManagerFactory

final class HibernateOrderStore @Inject private(
   protected val schedulerId: SchedulerId,
   protected val entityManagerFactory: EntityManagerFactory)
extends AbstractHibernateStore[OrderPersistentState, OrderEntity]
with OrderEntityConverter
