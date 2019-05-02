package com.sos.scheduler.engine.kernel.order.jobchain

import com.google.inject.Injector
import com.sos.scheduler.engine.base.utils.ScalaUtils._
import com.sos.scheduler.engine.common.guice.GuiceImplicits._
import com.sos.scheduler.engine.common.scalautil.Collections.emptyToNone
import com.sos.scheduler.engine.common.time.ScalaTime.RichDuration
import com.sos.scheduler.engine.cplusplus.runtime.annotation.ForCpp
import com.sos.scheduler.engine.cplusplus.runtime.{CppProxyWithSister, Sister, SisterType}
import com.sos.scheduler.engine.data.filebased.FileBasedType
import com.sos.scheduler.engine.data.jobchain.JobChainNodeAction.next_state
import com.sos.scheduler.engine.data.jobchain.{JobChainDetailed, JobChainObstacle, JobChainOverview, JobChainPath, JobChainPersistentState, JobChainState, NodeId}
import com.sos.scheduler.engine.data.message.MessageCode
import com.sos.scheduler.engine.data.order.OrderId
import com.sos.scheduler.engine.data.processclass.ProcessClassPath
import com.sos.scheduler.engine.data.queries.{JobChainNodeQuery, QueryableJobChain}
import com.sos.scheduler.engine.kernel.async.SchedulerThreadFutures.inSchedulerThread
import com.sos.scheduler.engine.kernel.cppproxy.{Job_chainC, OrderC}
import com.sos.scheduler.engine.kernel.filebased.FileBased
import com.sos.scheduler.engine.kernel.job.Job
import com.sos.scheduler.engine.kernel.order.jobchain.JobChain._
import com.sos.scheduler.engine.kernel.order.{DatabaseOrders, Order, OrderSubsystem}
import com.sos.scheduler.engine.kernel.persistence.hibernate.ScalaHibernate._
import com.sos.scheduler.engine.kernel.persistence.hibernate.{HibernateJobChainNodeStore, HibernateJobChainStore}
import com.sos.scheduler.engine.kernel.scheduler.HasInjector
import com.typesafe.config.Config
import java.nio.file.{Path, Paths}
import java.time.{Duration, Instant}
import javax.persistence.EntityManagerFactory
import scala.annotation.tailrec
import scala.collection.JavaConversions._
import scala.collection.{immutable, mutable}
import scala.concurrent.{Await, ExecutionContext, Future, TimeoutException}

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
        _edgeSet = (_nodeMap.values filter { _.action == next_state } map { o ⇒ o.nodeId → o.nextNodeId }).toSet
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
    def state = JobChain.this.state
  }

  private[kernel] def overview = {
    val (nonBlacklistedOrderCount, blacklistedOrderCount) = blockingOrderCount  // Synchronous database query
    JobChainOverview(
      path = path,
      fileBasedState = fileBasedState,
      state = state,
      jobOrJobChainNodeCount = jobOrJobChainNodeCount,
      nonBlacklistedOrderCount = nonBlacklistedOrderCount,
      blacklistedOrderCount = blacklistedOrderCount,
      hasJobChainNodes = hasJobChainNodes,
      isDistributed = isDistributed,
      orderLimit = orderLimitOption,
      title = cppProxy.title,
      orderIdSpaceName = orderIdSpaceNameOption,
      defaultProcessClassPath = defaultProcessClassPathOption,
      fileWatchingProcessClassPath = fileWatchingProcessClassPathOption,
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
  }

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

  private[kernel] def detailed =
    JobChainDetailed(
      overview = overview,
      nodes = nodes map { _.overview },
      fileOrderSources = for ((o, i) <- orderSourceDefinitions.zipWithIndex) yield
        JobChainDetailed.FileOrderSource(
          directory = o.directory,
          regex = o.regex,
          repeat = o.repeat,
          delayAfterError = o.delayAfterError,
          alertWhenDirectoryMissing = o.alertWhenDirectoryMissing,
          files =
            for ((file, mod) <- cppProxy.java_file_order_source_files(i).toVector zip cppProxy.java_file_order_source_files_last_modified(i)) yield
              JobChainDetailed.FileOrderSourceFile(Paths.get(file), Instant.ofEpochSecond(mod))),
      blacklistedOrders = cppProxy.blacklistedOrderIds.map(o => order(o).overview).toVector)

  private[order] def refersToJob(job: Job): Boolean = nodes exists {
    case n: SimpleJobNode => n.getJob eq job
    case _ => false
  }

  def jobNodes(query: JobChainNodeQuery): Iterator[JobNode] =
    inSchedulerThread {
      _jobNodes.iterator filter { o ⇒ query.matches(o.queryable) }
    }

  private lazy val _jobNodes: immutable.Seq[JobNode] = nodes collect { case o: JobNode ⇒ o }

  def jobNodes: immutable.Seq[JobNode] =
    inSchedulerThread { _jobNodes }

  def node(o: NodeId): Node =
    inSchedulerThread { _nodeMap(o) }

  private[kernel] def nodeMap: Map[NodeId, Node] =
    inSchedulerThread { _nodeMap }

  private lazy val _nodeMap: Map[NodeId, Node] =
    (nodes map { n ⇒ n.nodeId → n }).toMap withDefault { o ⇒ throw new NoSuchElementException(s"No JobChainNode for '${o.string}'")}

  private[order] lazy val nodes: immutable.Seq[Node] =
    (cppProxy.java_nodes map { _.asInstanceOf[CppProxyWithSister[_]].getSister.asInstanceOf[Node] }).toVector

  private lazy val jobOrJobChainNodeCount: Int =
    nodes count {
      case _: JobNode => true
      case _: NestedJobChainNode => true
      case _ => false
    }

  private lazy val orderSourceDefinitions: immutable.Seq[OrderSourceDefinition] =
    for (i <- 0 until cppProxy.order_source_count) yield
      OrderSourceDefinition(
        directory = Paths.get(cppProxy.java_file_order_source_directory(i)),
        regex = cppProxy.java_file_order_source_regex(i),
        repeat = Duration.ofMillis(cppProxy.java_file_order_source_repeat_millis(i)),
        delayAfterError = Duration.ofMillis(cppProxy.java_file_order_source_delay_after_error_millis(i)),
        alertWhenDirectoryMissing = cppProxy.java_file_order_source_alert_when_directory_missing(i))

  private def blockingOrderCount: (Int, Int) =
    if (isDistributed) {
      val configKey = "jobscheduler.master.database.read-timeout"
      val timeout = injector.instance[Config].getDuration(configKey).toFiniteDuration
      implicit val ec = injector.instance[ExecutionContext]
      val seqFuture = Future.sequence(Vector(
        injector.instance[DatabaseOrders].distributedNonBlacklistedOrderCount(path),
        injector.instance[DatabaseOrders].distributedBlacklistedOrderCount(path)))
      val seq =
        ( try Await.ready(seqFuture, timeout).value.get
          catch { case _: TimeoutException =>
            throw new TimeoutException(s"nonBlacklistedOrderCount: Database query did not respond before $configKey=$configKey")
          }
        ).get
      (seq(0), seq(1))
    } else
      (cppProxy.nondistributed_order_count, cppProxy.nondistributed_blacklisted_order_count)

  def order(id: OrderId) =
    inSchedulerThread {
      orderOption(id)
    } getOrElse {
      throw new NoSuchElementException(messageCodeHandler(MessageCode("SCHEDULER-161"), FileBasedType.Order, (path orderKey id).string))
    }

  private[kernel] def orderOption(id: OrderId): Option[Order] =
    Option(cppProxy.order_or_null(id.string)) map { _.getSister }

  private[order] def orderIterator(nodeId: NodeId): Iterator[Order] =
    orderIterator filter (_.nodeId == nodeId)

  private[order] def orderIterator: Iterator[Order] =
    cppProxy.java_orders.toIterator map { _.asInstanceOf[OrderC].getSister }

  private[kernel] def state: JobChainState =
    JobChainState.values()(cppProxy.state)

  private lazy val hasJobChainNodes: Boolean =
    nodes exists {
      case _: NestedJobChainNode => true
      case _ => false
    }

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

  private case class OrderSourceDefinition(
    directory: Path,
    regex: String,
    repeat: Duration,
    delayAfterError: Duration,
    alertWhenDirectoryMissing: Boolean)
}
