package com.sos.scheduler.engine.kernel.order.jobchain

import com.google.inject.Injector
import com.sos.scheduler.engine.cplusplus.runtime.Sister
import com.sos.scheduler.engine.cplusplus.runtime.annotation.ForCpp
import com.sos.scheduler.engine.data.folder.JobChainPath
import com.sos.scheduler.engine.data.order.OrderState
import com.sos.scheduler.engine.data.order.jobchain.{JobChainNodeAction, JobChainNodePersistentState}
import com.sos.scheduler.engine.kernel.cppproxy.NodeCI
import com.sos.scheduler.engine.kernel.persistence.hibernate.HibernateJobChainNodeStore
import com.sos.scheduler.engine.kernel.persistence.hibernate.ScalaHibernate._
import javax.persistence.EntityManagerFactory

/** @author Zschimmer.sos */
@ForCpp
class Node(cppProxy: NodeCI, injector: Injector) extends Sister {

  private var savedPersistentState: JobChainNodePersistentState = null

  def onCppProxyInvalidated() {}

  @ForCpp def persistState() {
    val p = persistentState
    if (p != savedPersistentState) {
      transaction(entityManagerFactory) { implicit entityManager =>
        persistentStateStore.store(p)
      }
      savedPersistentState = p
    }
  }

  protected def persistentStateStore = injector.getInstance(classOf[HibernateJobChainNodeStore])

  protected def entityManagerFactory = injector.getInstance(classOf[EntityManagerFactory])

  final def persistentState = new JobChainNodePersistentState(jobChainPath, orderState, action)

  final def jobChainPath = JobChainPath.of(cppProxy.job_chain_path)

  final def orderState = OrderState.of(cppProxy.string_order_state)

  final def nextState = OrderState.of(cppProxy.string_next_state)

  final def errorState = OrderState.of(cppProxy.string_error_state)

  final def action = JobChainNodeAction.ofCppName(cppProxy.string_action)

  final def action_=(o: JobChainNodeAction) {
    cppProxy.set_action(o.toCppName)
  }
}
