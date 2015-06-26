package com.sos.scheduler.engine.agent.data

import com.sos.scheduler.engine.data.base.IsString
import org.jetbrains.annotations.TestOnly
import scala.math.abs

/**
 * @author Joacim Zschimmer
 */
final case class AgentProcessId(value: Long) extends IsString {
  import com.sos.scheduler.engine.agent.data.AgentProcessId._

  def string = s"$index-$salt"

  /**
   *  Meaningless, just a hint for debugging.
   */
  @TestOnly
  def index = value / Factor

  override def toString = s"AgentProcessId($index-$salt)"

  private def salt = abs(value) % Factor
}

object AgentProcessId extends IsString.HasJsonFormat[AgentProcessId] {

  private val Factor = 1000*1000*1000L  // So the index is human readable if C++ Scheduler displays Process_id as 64bit integer.

  /**
   * Accepts a plain long number or the more readable form "index-salt".
   */
  def apply(string: String): AgentProcessId =
    try {
      val minus = string.length - (string.reverse indexOf '-') - 1
      if (minus == 0 | minus == string.length) {
        val number = string.toLong
        require(number.toString == string)
        new AgentProcessId(number)
      } else {
        val index = string take minus
        val salt = string drop (minus + 1)
        val result = AgentProcessId(index.toLong, salt.toLong)
        require(result.string == string)
        result
      }
    } catch {
      case _: Exception ⇒ throw new IllegalArgumentException(s"Invalid AgentProcessId($string)")
    }

  def apply(index: Long, salt: Long) = {
    val sign = if (index < 0) -1 else +1
    new AgentProcessId(index * Factor + sign * abs(salt) % Factor)
  }
}
