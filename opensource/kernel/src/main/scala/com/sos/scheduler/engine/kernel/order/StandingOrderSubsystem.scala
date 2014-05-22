package com.sos.scheduler.engine.kernel.order

import com.sos.scheduler.engine.data.filebased.FileBasedType
import com.sos.scheduler.engine.data.order.OrderKey
import com.sos.scheduler.engine.kernel.async.SchedulerThreadCallQueue
import com.sos.scheduler.engine.kernel.cppproxy.{OrderC, Standing_order_subsystemC}
import com.sos.scheduler.engine.kernel.filebased.FileBasedSubsystem
import com.sos.scheduler.engine.kernel.persistence.hibernate.HibernateOrderStore
import javax.inject.{Provider, Inject, Singleton}
import javax.persistence.EntityManagerFactory

@Singleton
final class StandingOrderSubsystem @Inject private(
  protected[this] val cppProxy: Standing_order_subsystemC,
  implicit val schedulerThreadCallQueue: SchedulerThreadCallQueue,
  entityManagerFactoryProvider: Provider[EntityManagerFactory],
  orderStoreProvider: Provider[HibernateOrderStore])
extends FileBasedSubsystem {

  type ThisSubsystem = StandingOrderSubsystem
  type ThisFileBased = Order
  type ThisFile_basedC = OrderC

  val description = StandingOrderSubsystem

  private[order] lazy val entityManagerFactory = entityManagerFactoryProvider.get
  private[order] lazy val orderStore = orderStoreProvider.get
}


object StandingOrderSubsystem extends FileBasedSubsystem.AbstractDesription[StandingOrderSubsystem, OrderKey, Order] {
  val fileBasedType = FileBasedType.order
  val stringToPath = { o: String ⇒ OrderKey(o) }
}
