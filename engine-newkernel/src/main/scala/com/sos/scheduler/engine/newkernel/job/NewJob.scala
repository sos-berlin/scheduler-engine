package com.sos.scheduler.engine.newkernel.job

import com.sos.scheduler.engine.common.async.CallQueue
import com.sos.scheduler.engine.common.scalautil.Logger
import com.sos.scheduler.engine.data.job.{JobPath, ReturnCode, TaskEnded, TaskId, TaskStarted}
import com.sos.scheduler.engine.eventbus.EventBus
import com.sos.scheduler.engine.newkernel.job.NewJob._
import com.sos.scheduler.engine.newkernel.job.commands.{SomeJobCommand, StopJobCommand}
import com.sos.scheduler.engine.newkernel.schedule.IntervalSelector
import com.sos.scheduler.engine.newkernel.utils.TimedCallHolder
import org.joda.time.Instant.now
import scala.collection.mutable

final class NewJob(
    val path: JobPath,
    val configuration: JobConfiguration,
    eventBus: EventBus,
    callQueue: CallQueue) {

  import configuration.{schedule, script}

  private val currentIntervalSelector = new IntervalSelector(schedule, callQueue)
  private var startTaskCallHolder = new TimedCallHolder(callQueue)
  private val tasks = mutable.HashMap[TaskId, ShellTask]()
  private var taskIdCounter = 0

  def close(): Unit = {
    currentIntervalSelector.close()
  }

  def activate(): Unit = {
    currentIntervalSelector.start()
    startTaskAtNextStartTime()
  }

  def executeCommand(o: SomeJobCommand): Unit = {
    o match {
      case StopJobCommand => stop()
    }
  }

  def onTaskTerminated(task: ShellTask): Unit = {
    eventBus publish new TaskEnded(task.id, path, ReturnCode(0))
    startTaskAtNextStartTime()
  }

  private def stop(): Unit = {
    // Alle Tasks beenden
    // state = JobState.stopping
  }

  private def startTaskAtNextStartTime(): Unit = {
    schedule.nextInstant(now()) match {
      case Some(t) =>
        logger debug s"nextInstant=$t"
        startTaskCallHolder.enqueue(t, "NewJob.startScheduledTask") {
          startScheduledTask()
        }
      case None =>
        logger debug "No nextInstant"
    }
  }

  private def startScheduledTask(): Unit = {
    startTask()
  }

  def startTask(): TaskId = {
    taskIdCounter += 1
    val task = script match {
      case script: ShellScript =>
        new ShellTask(TaskId(taskIdCounter), script, this, callQueue)
    }
    tasks += task.id -> task
    task.start()
    eventBus publish new TaskStarted(task.id, path)
    task.id
  }

  def tryToEndATask(): Boolean =
    ???

  def task(id: TaskId) = tasks.getOrElse(id, sys.error(s"Unknown TaskId ${id.string} for job ${path.string}"))

  def canBeRemovedNow =
    !hasTask

  def hasTask = tasks.nonEmpty

/*
  private def xx() {
    Event außer mit Klasse auch mit einem Schlüssel (JobPath) registrieren.
    Der EventBus kann dann einen Handler aus (Klasse, Schlüssel) auswählen und findet
    so den einen Handler von 15000 Jobs.
    case class TerminatedEvent(key: JobPath) extends KeyedEvent

    eventBus.registerHandler[ShellTaskTerminatedEvent](Set(path), his) {
      //..
    }
    eventBus.unregisterHandlersFor(this)
  }
*/
}

private object NewJob {
  private val logger = Logger(getClass)
}
