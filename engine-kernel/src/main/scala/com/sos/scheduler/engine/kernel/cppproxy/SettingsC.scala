package com.sos.scheduler.engine.kernel.cppproxy

import com.sos.scheduler.engine.cplusplus.runtime.CppProxy
import com.sos.scheduler.engine.cplusplus.runtime.annotation.{CppField, CppClass}

/**
 * @author Joacim Zschimmer
 */
@CppClass(clas = "sos::scheduler::Settings", directory = "scheduler", include = "spooler.h")
trait SettingsC extends CppProxy {
  def set(number: Int, value: String): Unit
  def is_freezed: Boolean

  @CppField
  def _db_name: String

  @CppField
  def _job_java_options: String

  @CppField
  def _job_java_classpath: String

  @CppField
  def _html_dir: String

  @CppField
  def _keep_order_content_on_reschedule: Boolean

  @CppField
  def _max_length_of_blob_entry: Int

  @CppField
  def _use_java_persistence: Boolean

  @CppField
  def _order_distributed_balanced: Boolean

  @CppField
  def _supervisor_configuration_polling_interval: Int

  @CppField
  def _cluster_restart_after_emergency_abort: Boolean

  @CppField
  def _use_old_microscheduling_for_jobs: Boolean

  @CppField
  def _use_old_microscheduling_for_tasks: Boolean

  @CppField
  def _always_create_database_tables: Boolean  // For tests only to suppress java error messages

  @CppField
  def _http_port: Int

  @CppField
  def _remote_scheduler_connect_retry_delay: Int

  @CppField
  def _web_directory: String
}
