package com.sos.scheduler.engine.kernel.order.jobchain

import com.google.inject.Injector
import com.sos.jobscheduler.base.utils.ScalaUtils._
import com.sos.jobscheduler.common.guice.GuiceImplicits._
import com.sos.jobscheduler.common.scalautil.Collections.emptyToNone
import com.sos.jobscheduler.data.message.MessageCode
import com.sos.scheduler.engine.cplusplus.runtime.annotation.ForCpp
import com.sos.scheduler.engine.cplusplus.runtime.{CppProxyWithSister, Sister, SisterType}
import com.sos.scheduler.engine.data.filebased.FileBasedType
import com.sos.scheduler.engine.data.jobchain.JobChainNodeAction.next_state
import com.sos.scheduler.engine.data.jobchain.{JobChainDetailed, JobChainObstacle, JobChainOverview, JobChainPath, JobChainPersistentState, JobChainState, NodeId}
import com.sos.scheduler.engine.data.order.OrderId
import com.sos.scheduler.engine.data.processclass.ProcessClassPath
import com.sos.scheduler.engine.data.queries.{JobChainNodeQuery, QueryableJobChain}
import com.sos.scheduler.engine.kernel.async.SchedulerThreadFutures.inSchedulerThread
import com.sos.scheduler.engine.kernel.cppproxy.{Job_chainC, OrderC}
import com.sos.scheduler.engine.kernel.filebased.FileBased
import com.sos.scheduler.engine.kernel.job.Job
import com.sos.scheduler.engine.kernel.order.jobchain.JobChain._
import com.sos.scheduler.engine.kernel.order.{Order, OrderSubsystem}
import com.sos.scheduler.engine.kernel.persistence.hibernate.ScalaHibernate._
import com.sos.scheduler.engine.kernel.persistence.hibernate.{HibernateJobChainNodeStore, HibernateJobChainStore}
import com.sos.scheduler.engine.kernel.scheduler.HasInjector
import javax.persistence.EntityManagerFactory
import scala.annotation.tailrec
import scala.collection.JavaConversions._
import scala.collection.{immutable, mutable}

@ForCpp
private[engine] final class JobChain(
  protected[this] val cppProxy: Job_chainC,
  protected[kernel] val subsystem: OrderSubsystem,
  injector: Injector)
extends FileBased
with UnmodifiableJobChain {

  protected type Self = JobChain
  type ThisPath = JobChainPath

  private object cppPredecessors {
    private var _edgeSet: Set[(NodeId, NodeId)] = null
    private val predecessorsMap = mutable.Map[String, java.util.ArrayList[String]]() withDefault { nodeIdString ⇒
      if (_edgeSet == null) {
        _edgeSet = (nodeMap.values filter { _.action == next_state } map { o ⇒ o.nodeId → o.nextNodeId }).toSet
      }
      val result = new java.util.ArrayList[String]
      result.addAll(allPredecessors(_edgeSet, NodeId(nodeIdString)) map { _.string } )
      result
    }

    def apply(orderStateString: String) = predecessorsMap(orderStateString)

    def invalidate(): Unit = {
      predecessorsMap.clear()
      _edgeSet = null
    }
  }

  private[kernel] val queryable = new QueryableJobChain {
    def path = JobChain.this.path
    def isDistributed = JobChain.this.isDistributed
  }

  private[kernel] def overview = JobChainOverview(
    path = path,
    fileBasedState = fileBasedState,
    isDistributed = isDistributed,
    orderLimit = orderLimitOption,
    orderIdSpaceName = orderIdSpaceNameOption,
    obstacles = {
      import JobChainObstacle._
      val builder = Set.newBuilder[JobChainObstacle]
      emptyToNone(fileBasedObstacles) switch {
        case Some(o) ⇒ builder += FileBasedObstacles(o)
      }
      if (state == JobChainState.stopped) {
        builder += Stopped
      }
      orderLimitOption switch {
        case Some(limit) ⇒ builder += OrderLimitReached(limit)
      }
      builder.result
    })

  def stringToPath(o: String) =
    JobChainPath(o)

  def fileBasedType =
    FileBasedType.JobChain

  def onCppProxyInvalidated(): Unit = {}

  private implicit def entityManagerFactory: EntityManagerFactory =
    injector.getInstance(classOf[EntityManagerFactory])

  @ForCpp private def persistState(): Unit = {
    transaction { implicit entityManager =>
      persistentStateStore.store(persistentState)
    }
  }

  @ForCpp private def deletePersistentState(): Unit = {
    transaction { implicit entityManager =>
      persistentStateStore.delete(path)
      nodeStore.deleteAll(path)
    }
  }

  private def persistentState =
    JobChainPersistentState(path, state == JobChainState.stopped)

  private def persistentStateStore =
    injector.getInstance(classOf[HibernateJobChainStore])

  private def nodeStore =
    injector.getInstance(classOf[HibernateJobChainNodeStore])

  @ForCpp
  private def onNextStateActionChanged(): Unit = {
    cppPredecessors.invalidate()
  }

  /** All NodeId, which are skipped to given orderStateString */
  @ForCpp
  private def cppSkippedStates(orderStateString: String): java.util.ArrayList[String] = cppPredecessors(orderStateString)

  private[kernel] def details =
    JobChainDetailed(
      overview = overview,
      nodes = nodes map { _.overview })

  private[order] def refersToJob(job: Job): Boolean = nodes exists {
    case n: SimpleJobNode => n.getJob eq job
    case _ => false
  }

  def jobNodes(query: JobChainNodeQuery): Iterator[JobNode] =
    jobNodes.iterator filter { o ⇒ query.matches(o.queryable) }

  lazy val jobNodes: immutable.Seq[JobNode] =
    inSchedulerThread {
      nodes collect { case o: JobNode ⇒ o }
    }

  def node(o: NodeId): Node = nodeMap(o)

  private[kernel] lazy val nodeMap: Map[NodeId, Node] =
    inSchedulerThread {
      (nodes map { n ⇒ n.nodeId → n }).toMap withDefault { o ⇒ throw new NoSuchElementException(s"No JobChainNode for '${o.string}'")}
    }

  private[order] lazy val nodes: immutable.Seq[Node] =
    (cppProxy.java_nodes map { _.asInstanceOf[CppProxyWithSister[_]].getSister.asInstanceOf[Node] }).toVector

  def order(id: OrderId) =
    inSchedulerThread {
      orderOption(id)
    } getOrElse {
      throw new NoSuchElementException(messageCodeHandler(MessageCode("SCHEDULER-161"), FileBasedType.Order, (path orderKey id).string))
    }

  private[kernel] def orderOption(id: OrderId): Option[Order] =
    Option(cppProxy.order_or_null(id.string)) map { _.getSister }

  private[order] def orderIterator: Iterator[Order] =
    cppProxy.java_orders.toIterator map { _.asInstanceOf[OrderC].getSister }

  private[kernel] def state: JobChainState =
    JobChainState.values()(cppProxy.state)

  private lazy val orderLimitOption: Option[Int] = someUnless(cppProxy.max_orders, none = Int.MaxValue)

  private[order] def remove(): Unit = {
    cppProxy.remove()
  }

  private def orderIdSpaceNameOption: Option[String] =
    emptyToNone(cppProxy.order_id_space_name)

  private[kernel] lazy val isDistributed: Boolean = cppProxy.is_distributed

  private[order] def defaultProcessClassPathOption = emptyToNone(cppProxy.default_process_class_path) map ProcessClassPath.apply

  private[kernel] def fileWatchingProcessClassPathOption = emptyToNone(cppProxy.file_watching_process_class_path) map ProcessClassPath.apply

  private[kernel] def isNestingJobChain: Boolean =
    nodes exists { _.isInstanceOf[NestedJobChainNode] }

  private def messageCodeHandler = subsystem.messageCodeHandler
}

object JobChain {
  object Type extends SisterType[JobChain, Job_chainC] {
    def sister(proxy: Job_chainC, context: Sister) = {
      val injector = context.asInstanceOf[HasInjector].injector
      new JobChain(proxy, injector.instance[OrderSubsystem], injector)
    }
  }

  /** All transitive predecessors of a graph described by edges. */
  private[jobchain] def allPredecessors[A](edges: Iterable[(A, A)], from: A) = {
    val nodeToPredecessors: Map[A, Iterable[A]] = (
      edges groupBy { _._2 }
      mapValues { edges ⇒ (edges map { _._1 }).toVector }
      withDefaultValue Nil)
    @tailrec
    def f(intermediateResult: Set[A]): Set[A] = {
      val preds = for (node ← intermediateResult; pred ← nodeToPredecessors(node)) yield pred
      val result = intermediateResult ++ preds
      if (result.size > intermediateResult.size) f(result) else result
    }
    f(Set(from)) - from
  }
}
