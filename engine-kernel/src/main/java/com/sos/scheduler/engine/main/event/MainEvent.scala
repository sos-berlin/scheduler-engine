package com.sos.scheduler.engine.main.event

import com.sos.scheduler.engine.data.event.NoKeyEvent

/**
  * Event from [[com.sos.scheduler.engine.main.SchedulerThreadController]]
  * or [[com.sos.scheduler.engine.main.SchedulerThread]]. */
sealed trait MainEvent extends NoKeyEvent

case object SchedulerReadyEvent extends MainEvent

final class TerminatedEvent(exitCode: Int, throwableOption: Option[Throwable])
extends MainEvent

/**
  * JavaSubsystem has been closed.
  */
case object SchedulerClosed extends MainEvent
