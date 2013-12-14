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
import javax.persistence.EntityManager
import scala.collection.JavaConversions._
import scala.collection.immutable

@ForCpp
final class JobChain(cppProxy: Job_chainC, injector: Injector)
extends FileBased
with UnmodifiableJobchain {

  lazy val nodeMap: Map[OrderState, Node] =
    (nodes map { n => n.orderState -> n }).toMap

  lazy val nodes: immutable.Seq[Node] =
    immutable.Seq() ++ cppProxy.java_nodes

  def onCppProxyInvalidated() {}

  @ForCpp
  private def loadPersistentState() {
    transaction(entityManager) { implicit entityManager =>
      for (persistentState <- nodeStore.fetchAll(getPath); node <- nodeMap.get(persistentState.state)) {
        node.action = persistentState.action
      }
      for (persistentState <- persistentStateStore.tryFetch(getPath)) {
        isStopped = persistentState.isStopped
      }
    }
  }

  @ForCpp
  private def persistState() {
    transaction(entityManager) { implicit entityManager =>
      persistentStateStore.store(persistentState)
    }
  }

  @ForCpp
  private def deletePersistentState() {
    transaction(entityManager) { implicit entityManager =>
      persistentStateStore.delete(getPath)
      nodeStore.deleteAll(getPath)
    }
  }

  private def persistentState: JobChainPersistentState =
    JobChainPersistentState(getPath, isStopped)

  private def entityManager: EntityManager =
    injector.getInstance(classOf[EntityManager])

  //private def entityManagerFactory = injector.getInstance(classOf[EntityManagerFactory])

  private def persistentStateStore: HibernateJobChainStore =
    injector.getInstance(classOf[HibernateJobChainStore])

  private def nodeStore: HibernateJobChainNodeStore =
    injector.getInstance(classOf[HibernateJobChainNodeStore])

  def getName: String =
    cppProxy.name

  def getFileBasedType: FileBasedType =
    FileBasedType.jobChain

  def getPath: JobChainPath = 
    JobChainPath(cppProxy.path)

  def file: File =
    cppProxy.file match {
      case "" => sys.error(this+ " has no source file")
      case o => new File(o)
    }

  /** Markiert, dass das [[com.sos.scheduler.engine.kernel.folder.FileBased]] beim nächsten Verzeichnisabgleich neu geladen werden soll. */
  def forceFileReread() {
    cppProxy.set_force_file_reread()
  }

  def refersToJob(job: Job): Boolean =
    nodes exists {
      case n: JobNode => n.getJob eq job
      case _ => false
    }

  def jobNodes: immutable.Seq[JobNode] =
    nodes collect { case o: JobNode => o }

  def node(o: OrderState): Node =
    nodeMap(o)

  def order(id: OrderId): Order =
    orderOption(id) getOrElse sys.error(s"$toString does not contain order '$id'")

  @Nullable
  def orderOrNull(id: OrderId): Order =
    orderOption(id).orNull

  def orderOption(id: OrderId): Option[Order] =
    Option(cppProxy.order_or_null(id.asString)) map { _.getSister }

  def isStopped: Boolean =
    cppProxy.is_stopped

  def isStopped_=(o: Boolean) {
    cppProxy.set_stopped(o)
  }

  override def toString =
    classOf[JobChain].getSimpleName +" "+ getPath
}
