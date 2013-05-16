package com.sos.scheduler.engine.newkernel.job

import NewJob._
import com.sos.scheduler.engine.common.async.CallQueue
import com.sos.scheduler.engine.common.scalautil.Logger
import com.sos.scheduler.engine.data.job.{TaskStartedEvent, TaskEndedEvent, TaskId}
import com.sos.scheduler.engine.eventbus.EventBus
import com.sos.scheduler.engine.newkernel.schedule.IntervalSelector
import com.sos.scheduler.engine.newkernel.utils.TimedCallHolder
import org.joda.time.Instant.now
import scala.collection.mutable
import com.sos.scheduler.engine.kernel.job.JobState
import com.sos.scheduler.engine.newkernel.job.commands.{StopJobCommand, SomeJobCommand}
import com.sos.scheduler.engine.data.folder.JobPath

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

  def close() {
    currentIntervalSelector.close()
  }

  def activate() {
    currentIntervalSelector.start()
    startTaskAtNextStartTime()
  }

  def executeCommand(o: SomeJobCommand) {
    o match {
      case StopJobCommand => stop()
    }
  }

  def onTaskTerminated(task: ShellTask) {
    eventBus publish new TaskEndedEvent(task.id, path)
    startTaskAtNextStartTime()
  }

  private def stop() {
    // Alle Tasks beenden
    // state = JobState.stopping
  }

  private def startTaskAtNextStartTime() {
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

  private def startScheduledTask() {
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
    eventBus publish new TaskStartedEvent(task.id, path)
    task.id
  }

  def tryToEndATask(): Boolean =
    ???

  def task(id: TaskId) = tasks.getOrElse(id, sys.error(s"Unknown TaskId $id for job $path"))

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

object NewJob {
  private val logger = Logger[NewJob]
}