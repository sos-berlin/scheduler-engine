package com.sos.scheduler.engine.tests.jira.js1227

import com.sos.scheduler.engine.common.scalautil.HasCloser.implicits._
import com.sos.scheduler.engine.common.utils.FreeTcpPortFinder._
import com.sos.scheduler.engine.test.SchedulerTestUtils._
import com.sos.scheduler.engine.test.configuration.TestConfiguration
import com.sos.scheduler.engine.test.scala.ScalaSchedulerTest
import scala.collection.immutable
import scala.concurrent.Future

/**
 * Mixin for [[ScalaSchedulerTest]] setting up a cluster of schedulers for distributed order processing.
 * @author Joacim Zschimmer
 */
trait ClusterTest extends ScalaSchedulerTest with SharedDatabaseTest {

  /** Number of members including the own scheduler. */
  protected def clusterMemberCount: Int

  protected lazy val databaseTcpPort :: ownTcpPort :: otherTcpPorts = findRandomFreeTcpPorts(2 + 2 * clusterMemberCount)
  protected lazy val clusterMainArguments = List(s"-tcp-port=$ownTcpPort", s"-udp-port=$ownTcpPort", "-distributed-orders")

  override protected lazy val testConfiguration = TestConfiguration(
    getClass,
    mainArguments = clusterMainArguments,
    database = Some(databaseConfiguration))

  protected lazy val otherSchedulers: immutable.IndexedSeq[ClusterScheduler] = {
    val newClusterScheduler = instance[ClusterScheduler.Factory]
    (for (Seq(tcpPort, httpPort) ← otherTcpPorts.toVector grouped 2) yield
      newClusterScheduler(testEnvironment, controller, databaseConfiguration, tcpPort = tcpPort, httpPort = httpPort)
        .registerCloseable
    ).toVector
  }

  override protected def onSchedulerActivated(): Unit = {
    awaitSuccess(startOtherSchedulers())  // Erst starten, wenn unser Scheduler die Datenbanktabellen eingerichtet hat.
    super.onSchedulerActivated()
  }

  /**
   * @return Future, successful when all cluster members are active and therefore the cluster is ready to use
   */
  protected def startOtherSchedulers(): Future[Unit] = Future.sequence(otherSchedulers map { _.start() }) map { _ ⇒ () }
}
