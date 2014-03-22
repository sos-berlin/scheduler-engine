package com.sos.scheduler.engine.kernel.order

import com.sos.scheduler.engine.data.filebased.FileBasedType
import com.sos.scheduler.engine.data.order.OrderKey
import com.sos.scheduler.engine.kernel.cppproxy.{OrderC, Standing_order_subsystemC}
import com.sos.scheduler.engine.kernel.filebased.FileBasedSubsystem
import javax.inject.{Inject, Singleton}

@Singleton
final class StandingOrderSubsystem @Inject private(
  protected[this] val cppProxy: Standing_order_subsystemC)
extends FileBasedSubsystem {

  type MySubsystem = StandingOrderSubsystem
  type MyFileBased = Order
  type MyFile_basedC = OrderC

  val companion = StandingOrderSubsystem
}


object StandingOrderSubsystem extends FileBasedSubsystem.Companion[StandingOrderSubsystem, OrderKey, Order](FileBasedType.order, OrderKey.apply)
