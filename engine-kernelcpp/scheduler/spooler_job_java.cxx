#include "spooler.h"
#include "spooler_job_java.h"
#include "Timed_call.h"
#include "../javaproxy/com__sos__scheduler__engine__newkernel__job__CppNewJob.h"
#include "../javaproxy/com__sos__scheduler__engine__kernel__cppproxy__Variable_setC.h"
#include "../javaproxy/com__sos__scheduler__engine__kernel__cppproxy__Job_nodeC.h"

typedef javaproxy::com::sos::scheduler::engine::newkernel::job::CppNewJob CppNewJobJ;

namespace sos {
namespace scheduler {
namespace job {

struct Java_job : Job {

  private:
    Fill_zero _zero_;
    CppNewJobJ _cppNewJobJ;

  public:
    Java_job(Spooler* spooler) : 
        Job(spooler),
        _zero_(this+1),
        _cppNewJobJ(CppNewJobJ::new_instance(spooler->java_proxy_jobject(), java_sister()))
    {}



    // *** FILE_BASED ***

    void close() {
        return _cppNewJobJ.close();
    }

    void set_xml_bytes(const string& o) {
        _cppNewJobJ.setXmlBytes(o);
    }

    bool on_initialize() {
        return _cppNewJobJ.onInitialize();
    }

    bool on_load() {
        return _cppNewJobJ.onLoad();
    }

    bool on_activate() {
        return _cppNewJobJ.onActivate();
    }

    void on_prepare_to_remove() {
        return _cppNewJobJ.onPrepareToRemove();
    }

    bool can_be_removed_now() {
        return _cppNewJobJ.canBeRemovedNow();
    }

    bool on_remove_now() {
        _cppNewJobJ.onRemoveNow();
        return true;
    }

    z::Xc remove_error() {
        return z::Xc(_cppNewJobJ.removalError());
    }



    // *** STATE ***

    string title() {
        return _cppNewJobJ.title();
    }

    string description() const {
        return _cppNewJobJ.description();
    }

    void set_state_text(const string& text) {
        _cppNewJobJ.setStateText(text);
    }

    string state_text() const {
        return "(state_text is not implemented)";
    }

    void set_state_cmd(State_cmd cmd) {
        set_state_cmd(state_cmd_name(cmd));
    }

    void set_state_cmd(const string& cmd) {
        return _cppNewJobJ.executeStateCommand(cmd);
    }

    void stop(bool end_all_tasks) {
        return _cppNewJobJ.stop();
    }

    void stop_simply(bool end_all_tasks) {
        return _cppNewJobJ.stopSimply();
    }

    bool is_permanently_stopped() const {
        return _cppNewJobJ.isPermanentlyStopped();
    }

    State state() const {
        return as_state(_cppNewJobJ.stateString());
    }

    int64 next_start_time_millis() const  {
        z::throw_xc(Z_FUNCTION);
    }

    jlong next_possible_start_millis() const  {
        z::throw_xc(Z_FUNCTION);
    }

    xml::Element_ptr dom_element(const xml::Document_ptr& doc, const Show_what& w, Job_chain* job_chain_or_null) {
        return _cppNewJobJ.domElement(doc.ref(), job_chain_or_null? job_chain_or_null->java_sister() : NULL);
    }

    xml::Element_ptr why_dom_element(const xml::Document_ptr&) {
        z::throw_xc(Z_FUNCTION);
    }

    xml::Element_ptr read_history( const xml::Document_ptr& doc, int task_id, int n, const Show_what& show ) {
        return _cppNewJobJ.readHistory(doc.ref(), task_id, n);
    }



    // *** PROCESS CLASS ***

    void notify_a_process_is_available() {
        return _cppNewJobJ.onProcessIsIdle();
    }

    bool waiting_for_process() const {
        return _cppNewJobJ.isWaitingForProcess();
    }


    // *** SCHEDULE ***

    void set_schedule_dom(const xml::Element_ptr& e) {
        _cppNewJobJ.setScheduleDOM(e.ref());
    }

    void on_schedule_loaded() {
        _cppNewJobJ.onReferencedScheduleLoaded();
    }

    void on_schedule_modified() {
        _cppNewJobJ.onReferencedScheduleModified();
    }

    bool on_schedule_to_be_removed() {
        return _cppNewJobJ.onReferencedScheduleToBeRemoved();
    }

    void append_calendar_dom_elements( const xml::Element_ptr&, Show_calendar_options* ) {
        z::throw_xc(Z_FUNCTION);
    }

    bool on_monitors_loaded() { 
        return false; 
    }
    
    bool on_monitor_to_be_removed(Monitor*) {
        return false;
    }


    // *** TASK ***

    ptr<Task> start_task_(Com_variable_set* params, Com_variable_set* environment, const Time& at, bool force, const string& task_name, const string& web_service_name) {
        //_cppNewJobJ.startTask(params->java_proxy_jobject(), environment? environment->java_proxy_jobject() : NULL, at.millis(), force, task_name, web_service_name);
        return NULL;
    }

    void enqueue_taskPersistentState(const TaskPersistentStateJ& task) {
        _cppNewJobJ.enqueueTask(task);
    }

    void remove_running_task(Task* task) {
        return _cppNewJobJ.removeRunningTask(task->id());
    }

    bool try_to_end_task(Process_class_requestor*, Process_class*) {
        return _cppNewJobJ.tryToEndATask();
    }

    void kill_task(int task_id, bool immediately, const Duration& timeout) {
        return _cppNewJobJ.killTask(task_id, immediately);
    }

    bool queue_filled() {
        return _cppNewJobJ.hasTask();
    }



    // *** ORDER ***

    bool connect_job_node(job_chain::Job_node* node) {
        return _cppNewJobJ.connectJobNode(node->java_sister());
    }

    void disconnect_job_node( job_chain::Job_node* node) {
        return _cppNewJobJ.disconnectJobNode(node->java_sister());
    }

    bool is_in_job_chain() const {
        return _cppNewJobJ.isInJobChain();
    }

    void set_order_controlled() {
        return _cppNewJobJ.setOrderControlled();
    }

    void set_idle_timeout(const Duration& d) {
        return _cppNewJobJ.setIdleTimeout(d.millis());
    }

    void signal_earlier_order(const Time& next_time, const string& order_name, const string& function )  {
        return _cppNewJobJ.signalEarlierOrder(next_time.millis(), order_name, function);
    }

    void on_order_possibly_available() {
        return _cppNewJobJ.onOrderPossiblyAvailable();
    }

    int max_order_setbacks() const {
        return _cppNewJobJ.orderSetbackMaximum();
    }

    bool is_order_controlled() const {
        return false;    
    }
    
    bool enabled() const {
        return true;
    }
    
    int task_queue_length() const {
        return 0;
    }

    bool has_error() const {
        return false;
    }
    
    string error_code() const {
        return "";
    }
    
    string error_message() const {
        return "";
    }
    
    ptr<Com_job>& com_job(){ 
        z::throw_xc(Z_FUNCTION);
    }

    string script_text() const { 
        return "script_text() not implemented"; 
    }

    const Absolute_path& default_process_class_path() const { throw_xc("NOT-IMPLEMENTED"); }
    bool is_in_period(const Time& = Time::now()) { throw_xc("NOT-IMPLEMENTED"); }
    bool max_tasks_reached() const { throw_xc("NOT-IMPLEMENTED"); }
    int max_tasks() const { throw_xc("NOT-IMPLEMENTED"); }
    int running_tasks_count() const { throw_xc("NOT-IMPLEMENTED"); }
    vector<string> unavailable_lock_path_strings() const { throw_xc("NOT-IMPLEMENTED"); }
    bool is_task_ready_for_order(Process_class*) { throw_xc("NOT-IMPLEMENTED"); }
    ArrayListJ java_tasks() const { throw_xc("NOT-IMPLEMENTED"); }
};

ptr<Job> new_java_job(Spooler* spooler) {
    ptr<Java_job> result = Z_NEW(Java_job(spooler));
    return +result;
}

}}}
