package com.sos.scheduler.engine.kernel.order.jobchain

import com.google.inject.Injector
import com.sos.scheduler.engine.cplusplus.runtime.annotation.ForCpp
import com.sos.scheduler.engine.data.folder.FileBasedType
import com.sos.scheduler.engine.data.folder.JobChainPath
import com.sos.scheduler.engine.data.order.OrderId
import com.sos.scheduler.engine.data.order.jobchain.JobChainPersistentState
import com.sos.scheduler.engine.kernel.cppproxy.Job_chainC
import com.sos.scheduler.engine.kernel.folder.FileBased
import com.sos.scheduler.engine.kernel.job.Job
import com.sos.scheduler.engine.kernel.order.Order
import com.sos.scheduler.engine.kernel.persistence.hibernate.ScalaHibernate._
import com.sos.scheduler.engine.kernel.persistence.hibernate.{HibernateJobChainNodeStore, HibernateJobChainStore}
import java.io.File
import javax.annotation.Nullable
import javax.persistence.EntityManagerFactory
import scala.collection.JavaConversions._
import scala.collection.immutable

@ForCpp
final class JobChain(cppProxy: Job_chainC, injector: Injector)
extends FileBased
with UnmodifiableJobchain {

  private var savedPersistentState: JobChainPersistentState = null

  def onCppProxyInvalidated() {}

  @ForCpp def loadPersistentState() {
    transaction(entityManagerFactory) { implicit entityManager =>
      for (persistentState <- nodeStore.fetchAll(getPath); node <- nodeMap.get(persistentState.state)) {
        node.action = persistentState.action
      }
      for (persistentState <- persistentStateStore.tryFetch(getPath)) {
        savedPersistentState = persistentState
        isStopped = persistentState.isStopped
      }
    }
  }

  @ForCpp def persistState() {
    val p = persistentState
    if (p != savedPersistentState) {
      transaction(entityManagerFactory) { entityManager =>
        persistentStateStore.store(p)(entityManager)
      }
      savedPersistentState = p
    }
  }

  @ForCpp def deletePersistentState() {
    transaction(entityManagerFactory) { entityManager =>
      persistentStateStore.delete(getPath)(entityManager)
      nodeStore.deleteAll(getPath)(entityManager)
    }
    savedPersistentState = null
  }

  private def persistentState = JobChainPersistentState(getPath, isStopped)

  private def entityManagerFactory = injector.getInstance(classOf[EntityManagerFactory])

  private def persistentStateStore = injector.getInstance(classOf[HibernateJobChainStore])

  private def nodeStore = injector.getInstance(classOf[HibernateJobChainNodeStore])

  def getName = cppProxy.name

  def getFileBasedType = FileBasedType.jobChain

  def getPath = JobChainPath.of(cppProxy.path)

  def file = cppProxy.file match {
    case "" => sys.error(this+ " has no source file")
    case o => new File(o)
  }

  @Deprecated
  def setForceFileReread() {
    forceFileReread()
  }

  /** Markiert, dass das [[com.sos.scheduler.engine.kernel.folder.FileBased]] beim nächsten Verzeichnisabgleich neu geladen werden soll. */
  def forceFileReread() {
    cppProxy.set_force_file_reread()
  }

  def refersToJob(job: Job): Boolean = nodes exists {
    case n: JobNode => n.getJob eq job
    case _ => false
  }

  lazy val nodeMap = (nodes map { n => n.orderState -> n }).toMap

  lazy val nodes = immutable.Seq() ++ cppProxy.java_nodes

  def order(id: OrderId) = orderOption(id) getOrElse sys.error(this+" does not contain order '"+id+"'")

  @Nullable def orderOrNull(id: OrderId): Order = orderOption(id).orNull

  def orderOption(id: OrderId): Option[Order] = Option(cppProxy.order_or_null(id.asString)) map { _.getSister }

  def isStopped = cppProxy.is_stopped

  def isStopped_=(o: Boolean) {
    cppProxy.set_stopped(o)
  }

  override def toString = classOf[JobChain].getSimpleName +" "+ getPath
}
