package com.sos.scheduler.engine.tests.jira.js995

import com.sos.scheduler.engine.data.folder.JobPath
import com.sos.scheduler.engine.test.scala.ScalaSchedulerTest
import org.junit.Ignore
import org.junit.runner.RunWith
import org.scalatest.junit.JUnitRunner

/** JS-995 */
@Ignore("Nicht geloest")
@RunWith(classOf[JUnitRunner])
final class JS995IT extends ScalaSchedulerTest {

  test("Schedule wirkt, ob Job vor oder nach Schedule gelesen wird") {
    // Prüfen, ob Jobs trotz SCHEDULER-161 die richtige Startzeit aus dem <schedule> haben.
    //instance[JobSubsystem].job(aJobPath).nextInstantOption should equal
  }
}

private object JS995IT {
  private val aJobPath = JobPath.of("/a")
  private val bJobPath = JobPath.of("/b")
}
