package com.sos.scheduler.engine.kernel.persistence.hibernate

import com.sos.scheduler.engine.data.order.OrderStepPersistentState
import com.sos.scheduler.engine.persistence.entities.{OrderStepEntity, OrderStepEntityConverter}
import javax.inject.Inject
import javax.persistence.EntityManagerFactory

final class HibernateOrderStepStore @Inject private(protected val entityManagerFactory: EntityManagerFactory)
extends AbstractHibernateStore[OrderStepPersistentState, OrderStepPersistentState.Key, OrderStepEntity]
with OrderStepEntityConverter
