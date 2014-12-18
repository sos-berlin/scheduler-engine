package com.sos.scheduler.engine.tests.jira.js1227

import com.sos.scheduler.engine.test.database.H2DatabaseServer
import com.sos.scheduler.engine.test.scalatest.ScalaSchedulerTest

/**
 * Mixin for [[ScalaSchedulerTest]] providing a database to be shared among multiple schedulers.
 * @author Joacim Zschimmer
 */
trait SharedDatabaseTest extends ScalaSchedulerTest {

  protected def databaseTcpPort: Int

  protected lazy val databaseConfiguration = new H2DatabaseServer.Configuration {
    def directory = testEnvironment.databaseDirectory
    def tcpPort = databaseTcpPort
  }

  protected lazy val databaseServer = new H2DatabaseServer(databaseConfiguration)

  override protected def checkedBeforeAll(): Unit = {
    databaseServer.start()
    super.checkedBeforeAll()
  }

  onClose {
    // Nach Scheduler-Ende
    databaseServer.close()
  }
}
