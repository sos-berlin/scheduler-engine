package com.sos.scheduler.engine.data.order

import com.sos.jobscheduler.base.generic.GenericInt

/**
  * @author Joacim Zschimmer
  */
final case class OrderHistoryId(number: Int) extends GenericInt

object OrderHistoryId extends GenericInt.Companion[OrderHistoryId]
