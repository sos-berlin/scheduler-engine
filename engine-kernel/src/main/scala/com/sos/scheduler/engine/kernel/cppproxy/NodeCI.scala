package com.sos.scheduler.engine.kernel.cppproxy

import com.sos.scheduler.engine.cplusplus.runtime.annotation.CppExpression

trait NodeCI {
  def job_chain_path: String
  def string_order_state: String
  def string_next_state: String
  def string_error_state: String
  def string_action: String
  def set_action_string(o: String): Unit
  @CppExpression("$->delay().millis()")
  def delay: Long
}
