package com.sos.scheduler.engine.test

import com.sos.jobscheduler.common.scalautil.Closers.implicits._
import com.sos.jobscheduler.common.utils.FreeTcpPortFinder._
import com.sos.scheduler.engine.test.SchedulerTestUtils._
import com.sos.scheduler.engine.test.configuration.TestConfiguration
import com.sos.scheduler.engine.test.scalatest.ScalaSchedulerTest
import scala.collection.immutable
import scala.concurrent.Future

/**
 * Mixin for [[ScalaSchedulerTest]] setting up a cluster of schedulers for distributed order processing.
 * @author Joacim Zschimmer
 */
trait ClusterTest extends ScalaSchedulerTest with SharedDatabaseTest {

  /** Number of members including the own scheduler. */
  protected def clusterMemberCount: Int

  protected lazy val databaseTcpPort :: ownHttpPort :: otherPorts = findRandomFreeTcpPorts(2 + 2 * clusterMemberCount)
  private lazy val ownUdpPort = ownHttpPort  // Cluster uses HTTP port number as UDP post number to to suspend order. See JS-1227 (JS1227IT)

  override protected final lazy val testConfiguration = TestConfiguration(
    getClass,
    mainArguments = List(s"-udp-port=$ownUdpPort", s"-http-port=127.0.0.1:$ownHttpPort", "-distributed-orders"),
    database = Some(databaseConfiguration))

  protected lazy val otherSchedulers: immutable.IndexedSeq[ClusterScheduler] = {
    val newClusterScheduler = instance[ClusterScheduler.Factory]
    (for (Seq(httpPort, tcpPort) ← otherPorts.toVector grouped 2) yield
      newClusterScheduler(testEnvironment, controller, databaseConfiguration, httpPort = httpPort, tcpPort = tcpPort)
        .closeWithCloser
    ).toVector
  }

  override protected def onSchedulerActivated(): Unit = {
    awaitSuccess(startOtherSchedulers())  // Erst starten, wenn unser Scheduler die Datenbanktabellen eingerichtet hat.
    super.onSchedulerActivated()
  }

  /**
   * @return Future, successful when all cluster members are active and therefore the cluster is ready to use
   */
  private def startOtherSchedulers(): Future[Unit] = Future.sequence(otherSchedulers map { _.start() }) map { _ ⇒ () }
}
