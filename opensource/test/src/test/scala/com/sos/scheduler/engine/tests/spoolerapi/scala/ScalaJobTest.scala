package com.sos.scheduler.engine.tests.spoolerapi.scala

import scala.collection.JavaConversions._
import com.sos.scheduler.engine.data.folder.AbsolutePath
import com.sos.scheduler.engine.data.job.TaskEndedEvent
import com.sos.scheduler.engine.kernel.log.SchedulerLogLevel
import com.sos.scheduler.engine.test.scala._
import com.sos.scheduler.engine.test.scala.SchedulerTestImplicits._
import org.junit.runner.RunWith
import org.scalatest.junit.JUnitRunner

/** Prüft, ob alle Methoden eines Scala-Jobs aufgerufen werden.
 * Der Scala-Job wird mit den Log-Levels info und error gerufen, um den Aufruf von spooler_on_success() und spooler_on_error()
 * zu prüfen.
 * Der Job schreibt in Spooler.variables für jeden Log-Level und jeden Aufruf eine Variable mit der Anzahl der Aufrufe.*/
@RunWith(classOf[JUnitRunner])
final class ScalaJobTest extends ScalaSchedulerTest {
  import ScalaJobTest._
  private val eventPipe = controller.newEventPipe()

  controller.setTerminateOnError(false)
  controller.activateScheduler()

  List(SchedulerLogLevel.info, SchedulerLogLevel.error) foreach { logLevel =>
    test("Job with "+logLevel+" should call some methods") {
      scheduler.executeXml(startJobElem(logLevel))
      eventPipe.nextWithCondition[TaskEndedEvent] {_.getJobPath == jobPath}
      checkMethodCallCounters(logLevel)
    }
  }

  private def checkMethodCallCounters(logLevel: SchedulerLogLevel) {
    // Der Job schreibt in scheduler.variables, wie oft der Scheduler jede Methode aufgerufen hat.
    val LevelString = logLevel.getNumber.toString
    val result = scheduler.getVariables.toMap collect {
        case (VariableNamePattern(LevelString, call), value) => call -> value.toInt
    }
    assert(result === expectedCallFrequencies(logLevel))
  }
}

object ScalaJobTest {
  private val jobPath = new AbsolutePath("/scala")
  private val VariableNamePattern = """test[.](\d+)[.]([a-z_]+)""".r  // "test.0.spooler_process"

  private val expectedCallFrequencies = Map(
    SchedulerLogLevel.info -> Map(
      "spooler_init" -> 1,
      "spooler_exit" -> 1,
      "spooler_open" -> 1,
      "spooler_close" -> 1,
      "spooler_process" -> 2,
      "spooler_on_success" -> 1),
    SchedulerLogLevel.error -> Map(
      "spooler_init" -> 1,
      "spooler_exit" -> 1,
      "spooler_open" -> 1,
      "spooler_close" -> 1,
      "spooler_process" -> 2,
      "spooler_on_error" -> 1))

  private def startJobElem(logLevel: SchedulerLogLevel) =
    <start_job job={jobPath.toString}>
      <params>
          <param name="logLevel" value={logLevel.getNumber.toString}/>
      </params>
    </start_job>
}
