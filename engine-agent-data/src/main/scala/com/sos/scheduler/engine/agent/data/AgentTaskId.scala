package com.sos.scheduler.engine.agent.data

import com.sos.scheduler.engine.data.base.IsString
import org.jetbrains.annotations.TestOnly
import scala.math.abs

/**
 * @see C++ Process_id
 * @author Joacim Zschimmer
 */
final case class AgentTaskId(value: Long) extends IsString {
  import com.sos.scheduler.engine.agent.data.AgentTaskId._

  def string = s"$index-$salt"

  /**
   *  Meaningless, just a hint for debugging.
   */
  @TestOnly
  def index = value / Factor

  override def toString = s"AgentTaskId($index-$salt)"

  private def salt = abs(value) % Factor
}

object AgentTaskId extends IsString.HasJsonFormat[AgentTaskId] {

  private val Factor = 1000*1000*1000L  // So the index is human readable if C++ Scheduler displays Process_id as 64bit integer.

  /**
   * Accepts a plain long number or the more readable form "index-salt".
   */
  def apply(string: String): AgentTaskId =
    try {
      val minus = string.length - (string.reverse indexOf '-') - 1
      if (minus == 0 | minus == string.length) {
        val number = string.toLong
        require(number.toString == string)
        new AgentTaskId(number)
      } else {
        val index = string take minus
        val salt = string drop (minus + 1)
        val result = AgentTaskId(index.toLong, salt.toLong)
        require(result.string == string)
        result
      }
    } catch {
      case _: Exception ⇒ throw new IllegalArgumentException(s"Invalid AgentTaskId($string)")
    }

  def apply(index: Long, salt: Long) = {
    val sign = if (index < 0) -1 else +1
    new AgentTaskId(index * Factor + sign * abs(salt) % Factor)
  }
}
