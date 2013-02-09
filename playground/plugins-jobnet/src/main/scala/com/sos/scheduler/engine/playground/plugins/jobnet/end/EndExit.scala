package com.sos.scheduler.engine.playground.plugins.jobnet.end

import com.sos.scheduler.engine.playground.plugins.jobnet.scheduler.Order
import com.sos.scheduler.engine.playground.plugins.jobnet.node.Exit

object EndExit extends Exit {
  def orderStates = Set()

  def moveOrder(o: Order) = {
    o.end()
  }
}
