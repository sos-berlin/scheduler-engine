package com.sos.scheduler.engine.common.async

trait PoppableCallQueue extends CallQueue {

  def nonEmpty = !isEmpty

  def isEmpty: Boolean

  def isMature: Boolean

  def popMature(): Option[TimedCall[_]]
}
