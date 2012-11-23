package com.sos.scheduler.engine.kernel.persistence.hibernate

import com.sos.scheduler.engine.data.order.{OrderPersistent, OrderKey}
import com.sos.scheduler.engine.data.scheduler.SchedulerId
import com.sos.scheduler.engine.persistence.entities.{OrderEntityConverter, OrderEntity}
import javax.inject.Inject
import javax.persistence.EntityManagerFactory

final class HibernateOrderStore @Inject()(
   protected val schedulerId: SchedulerId,
   protected val entityManagerFactory: EntityManagerFactory)
extends AbstractHibernateStore[OrderPersistent, OrderKey, OrderEntity]
with OrderEntityConverter
