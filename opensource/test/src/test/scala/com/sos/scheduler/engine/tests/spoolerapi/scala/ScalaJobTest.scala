package com.sos.scheduler.engine.tests.spoolerapi.scala

import scala.collection.JavaConversions._
import com.sos.scheduler.engine.kernel.folder.AbsolutePath
import com.sos.scheduler.engine.kernel.job.events.TaskEndedEvent
import com.sos.scheduler.engine.test.scala._
import com.sos.scheduler.engine.test.scala.SchedulerTestImplicits._
import com.sos.scheduler.engine.kernel.log.SchedulerLogLevel
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

    test("xx") {
        runJob(SchedulerLogLevel.info)
        runJob(SchedulerLogLevel.error)
    }

    private def runJob(logLevel: SchedulerLogLevel) {
        scheduler.executeXml(startJobElem(logLevel))
        eventPipe.expectEvent(shortTimeout){e: TaskEndedEvent => e.getJobPath == jobPath}
        checkSchedulerVariables(logLevel)
    }

    private def startJobElem(logLevel: SchedulerLogLevel) =
        <start_job job={jobPath.toString}>
            <params>
                <param name="logLevel" value={logLevel.getNumber.toString}/>
            </params>
        </start_job>

    private def checkSchedulerVariables(logLevel: SchedulerLogLevel) {
        val LevelString = logLevel.getNumber.toString
        val result = scheduler.getVariables.toMap flatMap {_ match {
            case (VariableNamePattern(LevelString, call), value) => Some(call -> value.toInt)
            case _ => None
        }}
        assert(result === expectedCallFrequencies(logLevel))
    }
}

object ScalaJobTest {
    private def jobPath = new AbsolutePath("/scala")
    private val VariableNamePattern = """test[.](\d+)[.]([a-z_]+)""".r      // "test.0.spooler_process"

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

    private def v(l: SchedulerLogLevel, call: String) = "test."+ l.getNumber +"."+call
}
