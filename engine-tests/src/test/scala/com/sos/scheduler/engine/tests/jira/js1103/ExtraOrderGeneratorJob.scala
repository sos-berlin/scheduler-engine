package com.sos.scheduler.engine.tests.jira.js1103

import com.sos.jobscheduler.common.scalautil.SideEffect.ImplicitSideEffect

final class ExtraOrderGeneratorJob extends sos.spooler.Job_impl {

  override def spooler_process() = {
    val order = spooler_task.order
    val id = order.id
    val jobChain = order.job_chain
    for (i ← JS1103IT.ExtraIndices) {
      jobChain.add_order(newExtraOrder(s"$id-$i-extra"))
    }
    Thread.sleep(100)
    true
  }

  private def newExtraOrder(id: String) =
    spooler.create_order() sideEffect  { r ⇒
      r.set_id(id)
      r.set_state("400")
      r.set_ignore_max_orders(true)
    }
}
