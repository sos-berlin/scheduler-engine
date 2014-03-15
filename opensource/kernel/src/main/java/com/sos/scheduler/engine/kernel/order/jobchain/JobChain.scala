package com.sos.scheduler.engine.kernel.order.jobchain

import com.google.inject.Injector
import com.sos.scheduler.engine.cplusplus.runtime.annotation.ForCpp
import com.sos.scheduler.engine.data.folder.FileBasedType
import com.sos.scheduler.engine.data.folder.JobChainPath
import com.sos.scheduler.engine.data.order.jobchain.JobChainPersistentState
import com.sos.scheduler.engine.data.order.{OrderState, OrderId}
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
with UnmodifiableJobChain {

  type Path = JobChainPath

  def onCppProxyInvalidated() {}

  private implicit def entityManagerFactory =
    injector.getInstance(classOf[EntityManagerFactory])

  @ForCpp
  private def loadPersistentState() {
    transaction { implicit entityManager =>
      for (persistentState <- nodeStore.fetchAll(path); node <- nodeMap.get(persistentState.state)) {
        node.action = persistentState.action
      }
      for (persistentState <- persistentStateStore.tryFetch(path)) {
        isStopped = persistentState.isStopped
      }
    }
  }

  @ForCpp private def persistState() {
    transaction { implicit entityManager =>
      persistentStateStore.store(persistentState)
    }
  }

  @ForCpp private def deletePersistentState() {
    transaction { implicit entityManager =>
      persistentStateStore.delete(path)
      nodeStore.deleteAll(path)
    }
  }

  private def persistentState =
    JobChainPersistentState(path, isStopped)

  private def persistentStateStore =
    injector.getInstance(classOf[HibernateJobChainStore])

  private def nodeStore =
    injector.getInstance(classOf[HibernateJobChainNodeStore])

  def fileBasedType =
    FileBasedType.jobChain

  def name =
    cppProxy.name

  def path =
    JobChainPath(cppProxy.path)

  def file = cppProxy.file match {
    case "" => sys.error(s"$toString has no source file")
    case o => new File(o)
  }

  /** Markiert, dass das [[com.sos.scheduler.engine.kernel.folder.FileBased]] beim nächsten Verzeichnisabgleich neu geladen werden soll. */
  def forceFileReread() {
    cppProxy.set_force_file_reread()
  }

  def refersToJob(job: Job): Boolean = nodes exists {
    case n: JobNode => n.getJob eq job
    case _ => false
  }

  def jobNodes: immutable.Seq[JobNode] =
    nodes collect { case o: JobNode => o }

  def node(o: OrderState): Node =
    nodeMap(o)

  lazy val nodeMap: Map[OrderState, Node] =
    (nodes map { n => n.orderState -> n }).toMap

  lazy val nodes: immutable.Seq[Node] =
    immutable.Seq() ++ cppProxy.java_nodes

  def order(id: OrderId) =
    orderOption(id) getOrElse sys.error(s"$toString does not contain order '$id'")

  @Nullable def orderOrNull(id: OrderId): Order =
    orderOption(id).orNull

  def orderOption(id: OrderId): Option[Order] =
    Option(cppProxy.order_or_null(id.string)) map { _.getSister }

  def isStopped =
    cppProxy.is_stopped

  def isStopped_=(o: Boolean) {
    cppProxy.set_stopped(o)
  }

  private[order] def remove() {
    cppProxy.remove()
  }
}
