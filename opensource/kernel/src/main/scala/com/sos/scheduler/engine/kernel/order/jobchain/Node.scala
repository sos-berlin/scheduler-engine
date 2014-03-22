package com.sos.scheduler.engine.kernel.order.jobchain

import com.google.inject.Injector
import com.sos.scheduler.engine.cplusplus.runtime.Sister
import com.sos.scheduler.engine.cplusplus.runtime.annotation.ForCpp
import com.sos.scheduler.engine.data.jobchain.{JobChainPath, JobChainNodeAction, JobChainNodePersistentState}
import com.sos.scheduler.engine.data.order.OrderState
import com.sos.scheduler.engine.kernel.cppproxy.NodeCI
import com.sos.scheduler.engine.kernel.persistence.hibernate.HibernateJobChainNodeStore
import com.sos.scheduler.engine.kernel.persistence.hibernate.ScalaHibernate._
import javax.persistence.EntityManagerFactory

/** @author Zschimmer.sos */
@ForCpp
class Node(cppProxy: NodeCI, injector: Injector) extends Sister {

  def onCppProxyInvalidated() {}

  private implicit def entityManagerFactory =
    injector.getInstance(classOf[EntityManagerFactory])

  @ForCpp private def persistState() {
    transaction { implicit entityManager =>
      persistentStateStore.store(persistentState)
    }
  }

  protected def persistentStateStore =
    injector.getInstance(classOf[HibernateJobChainNodeStore])

  final def persistentState =
    new JobChainNodePersistentState(jobChainPath, orderState, action)

  final def jobChainPath =
    JobChainPath(cppProxy.job_chain_path)

  final def orderState =
    OrderState(cppProxy.string_order_state)

  final def nextState =
    OrderState(cppProxy.string_next_state)

  final def errorState =
    OrderState(cppProxy.string_error_state)

  final def action =
    JobChainNodeAction.ofCppName(cppProxy.string_action)

  final def action_=(o: JobChainNodeAction) {
    cppProxy.set_action_string(o.toCppName)
  }
}
