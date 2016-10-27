package com.sos.scheduler.engine.kernel.order

import com.google.inject.Injector
import com.sos.scheduler.engine.client.agent.SchedulerAgentClientFactory
import com.sos.scheduler.engine.data.filebased.FileBasedType
import com.sos.scheduler.engine.data.order.OrderKey
import com.sos.scheduler.engine.kernel.async.SchedulerThreadCallQueue
import com.sos.scheduler.engine.kernel.cppproxy.{OrderC, Standing_order_subsystemC}
import com.sos.scheduler.engine.kernel.filebased.FileBasedSubsystem
import com.sos.scheduler.engine.kernel.persistence.hibernate.HibernateOrderStore
import javax.inject.{Inject, Provider, Singleton}
import javax.persistence.EntityManagerFactory

@Singleton
private[kernel] final class StandingOrderSubsystem @Inject private(
  protected[this] val cppProxy: Standing_order_subsystemC,
  implicit val schedulerThreadCallQueue: SchedulerThreadCallQueue,
  entityManagerFactoryProvider: Provider[EntityManagerFactory],
  orderStoreProvider: Provider[HibernateOrderStore],
  agentClientFactoryProvider: Provider[SchedulerAgentClientFactory],
  protected val injector: Injector)
extends FileBasedSubsystem {

  type ThisSubsystemClient = StandingOrderSubsystemClient
  type ThisSubsystem = StandingOrderSubsystem
  type ThisFileBased = Order
  type ThisFile_basedC = OrderC
  type Path = OrderKey

  val companion = StandingOrderSubsystem
  private[kernel] def agentClientFactory = agentClientFactoryProvider.get

  private[order] def entityManagerFactory = entityManagerFactoryProvider.get
  private[order] def orderStore = orderStoreProvider.get
}


object StandingOrderSubsystem
extends FileBasedSubsystem.AbstractCompanion[StandingOrderSubsystemClient, StandingOrderSubsystem, OrderKey, Order] {

  val fileBasedType = FileBasedType.Order
  val stringToPath = { o: String ⇒ OrderKey(o) }
}
