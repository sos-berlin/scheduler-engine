package com.sos.scheduler.engine.kernel.cppproxy

import com.sos.scheduler.engine.cplusplus.runtime.CppProxyWithSister
import com.sos.scheduler.engine.cplusplus.runtime.annotation.{CppClass, CppExpression, CppField, CppThreadSafe}
import com.sos.scheduler.engine.kernel.Scheduler
import com.sos.scheduler.engine.kernel.http.SchedulerHttpRequest
import com.sos.scheduler.engine.kernel.http.SchedulerHttpResponse

@CppClass(clas = "sos::scheduler::Spooler", directory = "scheduler", include = "spooler.h")
trait SpoolerC extends CppProxyWithSister[Scheduler] {
  def log: Prefix_logC
  def id: String
  def id_for_db: String
  def http_url: String
  def name: String
  def param: String
  def variables: Variable_setC
  def udp_port: Int
  def tcp_port: Int
  def hostname: String
  def hostname_complete: String
  def include_path: String
  def temp_dir: String
  def state: Int
  def pid: Int
  def log_directory: String
  def is_service: Boolean
  def configuration_file_path: String
  def directory: String
  def local_configuration_directory: String
  def log_show_state(): Unit
  def execute_xml_string(xml: String): String
  def execute_xml_string_with_security_level(xml: String, security_level: String, clientHostName: String): String
  def java_execute_http(request: SchedulerHttpRequest, response: SchedulerHttpResponse): HttpResponseC
  def java_execute_http_with_security_level(request: SchedulerHttpRequest, response: SchedulerHttpResponse, security_level: String): HttpResponseC
  def cmd_pause(): Unit
  def cmd_continue(): Unit
  def cmd_terminate_after_error(function_name: String, message_text: String): Unit
  def cmd_terminate(restart: Boolean, timeout: Int, continue_exclusive_operation: String, terminate_all_schedulers: Boolean): Unit
  def cmd_terminate(restart: Boolean, timeout: Int, continue_exclusive_operation: String): Unit
  def cmd_terminate(restart: Boolean, timeout: Int): Unit
  def cmd_terminate(restart: Boolean): Unit
  def cmd_terminate(): Unit
  def cmd_terminate_and_restart(timeout: Int): Unit
  def cmd_let_run_terminate_and_restart(): Unit
  def abort_immediately_after_distribution_error(debug_text: String): Unit
  def abort_immediately(restart: Boolean, message_text: String): Unit
  def abort_now(restart: Boolean): Unit
  def execute_state_cmd(): Unit
  def is_termination_state_cmd: Boolean
  def name_is_valid(name: String): Boolean
  def check_name(name: String): Unit
  // Cluster
  def check_cluster(): Unit
  def assert_is_activated(function: String): Unit
  def is_cluster: Boolean
  def cluster_is_active: Boolean
  def has_exclusiveness: Boolean
  def orders_are_distributed: Boolean
  def assert_are_orders_distributed(message_text: String): Unit
  def cluster_member_id: String
  def distributed_member_id: String
  def db_distributed_member_id: String
  def modifiable_settings: SettingsC
  def settings: SettingsC
  @CppThreadSafe def signal(): Unit
  def db: DatabaseC
  def folder_subsystem: Folder_subsystemC
  def job_subsystem: Job_subsystemC
  def job_subsystem_or_null: Job_subsystemC
  def order_subsystem: Order_subsystemC
  def task_subsystem: Task_subsystemC
  def lock_subsystem: Lock_subsystemC
  def process_class_subsystem: Process_class_subsystemC
  def schedule_subsystem: Schedule_subsystemC
  def standing_order_subsystem: Standing_order_subsystemC
  def has_any_task: Boolean
  @CppThreadSafe def write_to_scheduler_log(category: String, text: String): Unit
  def time_zone_name: String
  def supervisor_uri: String
  def last_mail_failed: Boolean

  @CppExpression("$->_mail_defaults.to_java()")
  def mailDefaults: java.util.List[String]
}
