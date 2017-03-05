package com.sos.scheduler.engine.data.order

import com.sos.jobscheduler.base.generic.IsString

final case class OrderId(string: String) extends IsString

object OrderId extends IsString.Companion[OrderId]
