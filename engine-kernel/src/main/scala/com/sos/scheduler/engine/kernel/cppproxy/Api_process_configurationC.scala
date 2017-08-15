package com.sos.scheduler.engine.kernel.cppproxy

import com.sos.scheduler.engine.cplusplus.runtime.CppProxy
import com.sos.scheduler.engine.cplusplus.runtime.annotation.{CppClass, CppExpression, CppField}

/**
 * @author Joacim Zschimmer
 */
@CppClass(clas="sos::scheduler::Api_process_configuration", directory="scheduler", include="spooler.h")
trait Api_process_configurationC extends CppProxy {

  @CppExpression("$->_controller_address.as_string()")
  def controllerAddressString: String

  @CppField
  def _remote_scheduler_address: String

  @CppField
  def _job_path: String

  @CppField
  def _task_id: Int

  @CppField
  def _has_api: Boolean

  @CppField
  def _is_thread: Boolean

  @CppField
  def _log_stdout_and_stderr: Boolean

  @CppField
  def _priority: String

  @CppField
  def _environment: Variable_setC

  @CppField
  def _credentials_key: String

  @CppField
  def _load_user_profile: Boolean

  @CppField
  def _java_options: String

  @CppField
  def _java_classpath: String
}
