package com.sos.scheduler.engine.agent.task

import scala.util.Random

/**
 * @author Joacim Zschimmer
 */
final case class RemoteTaskId(value: Long)

object RemoteTaskId {
  private val Factor = 1000*1000*1000

  /**
   * Delivers RemoteTaskId with recognizable increasing numbers.
   * The increasing number is meaningless.
   */
  def newGenerator(): Iterator[RemoteTaskId] =
    Iterator from 1 map { i ⇒ RemoteTaskId((Random.nextLong() / Factor * Factor) + (i % (Int.MaxValue - 1))) }
    //TODO Nur positive Zahlen, keine 0

}
