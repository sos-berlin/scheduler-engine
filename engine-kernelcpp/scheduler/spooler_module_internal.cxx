#include "spooler.h"
#include "../javaproxy/com__sos__scheduler__engine__kernel__async__CppCall.h"
#include "../javaproxy/com__sos__scheduler__engine__kernel__job__Task.h"
#include "../javaproxy/com__sos__scheduler__engine__kernel__job__internal__InternalModule.h"

typedef ::javaproxy::com::sos::scheduler::engine::kernel::job::internal::InternalModule InternalModuleJ;

namespace sos {
namespace scheduler {

struct Internal_module : Module
{
    private: string const _name;

    public: Internal_module(Spooler* spooler, Prefix_log* log, const string& name) :
        Module(spooler, (File_based*)NULL, "::NO-INCLUDE-PATH::", log, false),
        _name(name)
    {
        _kind = kind_internal;
        _set = true;
    }

    public: ptr<Module_instance> create_instance_impl(Process_class*, const string& remote_scheduler, Task*);

    public: string name() {
        return _name;
    }
};

struct Internal_module_instance : Module_instance, Scheduler_object
{
    DEFINE_SIMPLE_CALL(Internal_module_instance, Completed_call)

    struct TryJ_operation : Async_operation {
        private: string const _name;
        private: TryJ _result;

        public: TryJ_operation(const string& name) :
            _name(name)
        {}

        public: void complete(const TryJ& result) {
            _result = result;
        }

        public: const TryJ& result() const {
            return _result;
        }

        public: bool async_continue_(Continue_flags) { 
            return false; 
        }

        public: bool async_finished_() const { 
            return _result.get_jobject() != NULL; 
        }

        public: string async_state_text_() const { 
            return S() << "Call_operation(" + _name + ")"; 
        }

        public: string name() const {
            return _name;
        }
    };

    private: Fill_zero _zero_;
    private: InternalModuleJ const _internalModuleJ;
    private: Task* const _task;
    private: Prefix_log* _log;
    private: ptr<TryJ_operation> _operation;
    private: ptr<Completed_call> const _completed_call;

    public: Internal_module_instance(Internal_module* module, const string& remote_scheduler, Task* task) :
        Module_instance(module),
        _zero_(this+1),
        _internalModuleJ(InternalModuleJ::apply(module->_spooler->injectorJ(), module->name(), remote_scheduler, task->java_sister())),
        _completed_call(Z_NEW(Completed_call(this))),
        _task(task)
    {
    }

    public: Type_code scheduler_type_code() const {
        return type_internal_module;
    }

    public: Spooler* spooler() const {
        return _module->_spooler;
    }

    public: Prefix_log* log() const {
        return _log;
    }

    public: Variant call(const string& name) {
        z::throw_xc(Z_FUNCTION, name);
    }

    public: Variant call(const string& name, const Variant&, const Variant&) {
        z::throw_xc(Z_FUNCTION, name);
    }

    public: Async_operation* close__start() {
        start_operation("close");
        _internalModuleJ.close(_completed_call->java_sister());
        return _operation;
    }
    
    public: void close__end() {
        check_state_for_end("close");
        _operation->result().get();
    }

    public: Async_operation* begin__start() {
        start_operation("begin");
        _internalModuleJ.begin(_completed_call->java_sister());
        return _operation;
    }

    public: bool begin__end() {
        return bool_end_operation("begin");
    }

    public: Async_operation* step__start() {
        start_operation("step");
        _internalModuleJ.step(_completed_call->java_sister());
        return _operation;
    }

    public: Variant step__end() {
        return bool_end_operation("step");
    }

    public: Async_operation* end__start(bool success) {
        start_operation("end");
        _internalModuleJ.end(success, _completed_call->java_sister());
        return _operation;
    }

    public: void end__end() {
        end_operation("end");
    }

    public: Async_operation* call__start(const string& method) {
        start_operation("call");
        _internalModuleJ.call(method, _completed_call->java_sister());
        return _operation;
    }

    public: Variant call__end() {
        ObjectJ result = end_operation("call");
        return string_ends_with(_call_method, ")Z") ? ((BooleanJ)result).booleanValue() : Variant();
    }

    public: Async_operation* release__start() {
        start_operation("release");
        _internalModuleJ.release(_completed_call->java_sister());
        return _operation;
    }

    public: void release__end() {
        end_operation("release");
    }

    public: bool name_exists(const string& name) {
        return _internalModuleJ.nameExists(name);
    }
    
    public: bool loaded() { 
        return _load_called; 
    }

    public: bool callable() { 
        return true; 
    }

    private: void start_operation(const string& name) {
        if (_operation) z::throw_xc(Z_FUNCTION, "Duplicate start", name, _operation->name());
        _operation = Z_NEW(TryJ_operation(name));
    }

    private: bool bool_end_operation(const string& name) {
        return ((BooleanJ)end_operation(name)).booleanValue();
    }

    private: ObjectJ end_operation(const string& name) {
        check_state_for_end(name);
        ptr<TryJ_operation> op = _operation;
        _operation = NULL;
        return op->result().get();  // Try.get throws exception in case of Failure
    }

    private: void check_state_for_end(const string& name) {
        if (!_operation) z::throw_xc("SCHEDULER-191", name, "_operation==NULL");
        if (!_operation->async_finished()) z::throw_xc("SCHEDULER-191", name, _operation->async_state_text());
        if (name != _operation->name()) z::throw_xc("SCHEDULER-191", name, _operation->async_state_text());
    }

    public: void on_call(const Completed_call& call) {
        if (call.value().get_jobject() == NULL)  z::throw_xc(Z_FUNCTION, "NULL");
        _operation->complete((TryJ)call.value());
        _operation->async_finished_then_call();
    }
};


ptr<Module_instance> Internal_module::create_instance_impl(Process_class*, const string& remote_scheduler, Task* task) {
    ptr<Internal_module_instance> result = Z_NEW(Internal_module_instance(this, remote_scheduler, task));
    return +result;
}

ptr<Module> new_internal_module(Spooler* spooler, Prefix_log* log, const string& name) {
    ptr<Internal_module> result = Z_NEW(Internal_module(spooler, log, name));
    return +result;
}

}} // namespace
