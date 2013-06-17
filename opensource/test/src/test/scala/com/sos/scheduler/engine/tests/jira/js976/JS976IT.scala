package com.sos.scheduler.engine.tests.jira.js976

import com.sos.scheduler.engine.common.time.ScalaJoda._
import com.sos.scheduler.engine.test.configuration.{HostwareDatabaseConfiguration, DefaultDatabaseConfiguration, TestConfiguration}
import com.sos.scheduler.engine.test.scala.ScalaSchedulerTest
import com.sos.scheduler.engine.test.scala.SchedulerTestImplicits._
import org.junit.runner.RunWith
import org.scalatest.junit.JUnitRunner

/** Test setzt eine laufende MySQL-Datenbank mit wait_timeout=1 voraus.
  *
  * Erforderliche MySQL-Kommandos:
  *   create database scheduler;
  *   create user 'scheduler'@'localhost';
  *   create database scheduler;
  */
@RunWith(classOf[JUnitRunner])
final class JS976IT extends ScalaSchedulerTest {

  private val withMySQL = System.getProperty("JS-976") != null   // Test mit MySQL nur, wenn die System-Property gesetzt ist

  override lazy val testConfiguration = {
    val databaseConfiguration =
      if (withMySQL) HostwareDatabaseConfiguration("jdbc -class=com.mysql.jdbc.Driver -user=scheduler jdbc:mysql://127.0.0.1/scheduler")
      else DefaultDatabaseConfiguration()
    TestConfiguration(database = Some(databaseConfiguration))
  }

  if (withMySQL)
    test("JS-976") {
      sleep(3.s)
      scheduler executeXml <job_chain_node.modify job_chain="test" state="100" action="stop"/>
    }
}
