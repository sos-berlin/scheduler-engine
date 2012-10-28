package com.sos.scheduler.engine.kernel.order.jobchain

import com.google.inject.Injector
import com.sos.scheduler.engine.data.folder.FileBasedType
import com.sos.scheduler.engine.data.folder.JobChainPath
import com.sos.scheduler.engine.data.order.OrderId
import com.sos.scheduler.engine.kernel.cppproxy.Job_chainC
import com.sos.scheduler.engine.kernel.cppproxy.OrderC
import com.sos.scheduler.engine.kernel.folder.FileBased
import com.sos.scheduler.engine.kernel.job.Job
import com.sos.scheduler.engine.kernel.order.Order
import javax.annotation.Nullable
import scala.collection.JavaConversions._
import scala.collection.immutable

final class JobChain(cppProxy: Job_chainC, injector: Injector) extends FileBased with UnmodifiableJobchain {

  def onCppProxyInvalidated() {}

  def getName = cppProxy.name

  def getFileBasedType = FileBasedType.jobChain

  def getPath = JobChainPath.of(cppProxy.path)

  /** Markiert, dass das [[com.sos.scheduler.engine.kernel.folder.FileBased]] beim nächsten Verzeichnisabgleich neu geladen werden soll. */
  def setForceFileReread() {
    cppProxy.set_force_file_reread()
  }

  def refersToJob(job: Job): Boolean = nodes exists {
    case n: JobNode => n.getJob eq job
    case _ => false
  }

  def nodes: immutable.Seq[Node] = immutable.Seq() ++ cppProxy.java_nodes

  def order(id: OrderId) = cppProxy.order(id.asString).getSister

  @Nullable def orderOrNull(id: OrderId): Order = {
    val o: OrderC = cppProxy.order_or_null(id.asString)
    if (o == null) null else o.getSister
  }

  override def toString = classOf[JobChain].getSimpleName +" "+ getPath
}
