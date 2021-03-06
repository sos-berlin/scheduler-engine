// *** Generated by com.sos.scheduler.engine.cplusplus.generator ***

package com.sos.scheduler.engine.kernel.cppproxy;

@javax.annotation.Generated("C++/Java-Generator - SOS GmbH Berlin")
@SuppressWarnings({"unchecked", "rawtypes"})
final class TaskCImpl
extends com.sos.scheduler.engine.cplusplus.runtime.CppProxyImpl<com.sos.scheduler.engine.kernel.job.Task>
implements com.sos.scheduler.engine.kernel.cppproxy.TaskC {

    // <editor-fold defaultstate="collapsed" desc="Generated code - DO NOT EDIT">

    private TaskCImpl(com.sos.scheduler.engine.cplusplus.runtime.Sister context) { // Nur für JNI zugänglich
        setSister(com.sos.scheduler.engine.kernel.cppproxy.TaskC$.MODULE$.sisterType().sister(this, context));
    }

    @Override public long at_millis() {
        com.sos.scheduler.engine.cplusplus.runtime.CppProxy$.MODULE$.requireCppThread();
        try {
            return at_millis__native(cppReference());
        }
        catch (Exception x) { throw com.sos.scheduler.engine.cplusplus.runtime.CppProxies.propagateCppException(x, this); }
    }

    private static native long at_millis__native(long cppReference);


    @Override public java.lang.String cause_string() {
        com.sos.scheduler.engine.cplusplus.runtime.CppProxy$.MODULE$.requireCppThread();
        try {
            java.lang.String result = cause_string__native(cppReference());
            checkIsNotReleased(java.lang.String.class, result);
            return result;
        }
        catch (Exception x) { throw com.sos.scheduler.engine.cplusplus.runtime.CppProxies.propagateCppException(x, this); }
    }

    private static native java.lang.String cause_string__native(long cppReference);


    @Override public long enqueued_at_millis() {
        com.sos.scheduler.engine.cplusplus.runtime.CppProxy$.MODULE$.requireCppThread();
        try {
            return enqueued_at_millis__native(cppReference());
        }
        catch (Exception x) { throw com.sos.scheduler.engine.cplusplus.runtime.CppProxies.propagateCppException(x, this); }
    }

    private static native long enqueued_at_millis__native(long cppReference);


    @Override public int id() {
        com.sos.scheduler.engine.cplusplus.runtime.CppProxy$.MODULE$.requireCppThread();
        try {
            return id__native(cppReference());
        }
        catch (Exception x) { throw com.sos.scheduler.engine.cplusplus.runtime.CppProxies.propagateCppException(x, this); }
    }

    private static native int id__native(long cppReference);


    @Override public boolean is_waiting_for_remote_scheduler() {
        com.sos.scheduler.engine.cplusplus.runtime.CppProxy$.MODULE$.requireCppThread();
        try {
            return is_waiting_for_remote_scheduler__native(cppReference());
        }
        catch (Exception x) { throw com.sos.scheduler.engine.cplusplus.runtime.CppProxies.propagateCppException(x, this); }
    }

    private static native boolean is_waiting_for_remote_scheduler__native(long cppReference);


    @Override public com.sos.scheduler.engine.kernel.cppproxy.JobC job() {
        com.sos.scheduler.engine.cplusplus.runtime.CppProxy$.MODULE$.requireCppThread();
        try {
            com.sos.scheduler.engine.kernel.cppproxy.JobC result = job__native(cppReference());
            checkIsNotReleased(com.sos.scheduler.engine.kernel.cppproxy.JobC.class, result);
            return result;
        }
        catch (Exception x) { throw com.sos.scheduler.engine.cplusplus.runtime.CppProxies.propagateCppException(x, this); }
    }

    private static native com.sos.scheduler.engine.kernel.cppproxy.JobC job__native(long cppReference);


    @Override public java.lang.String job_path() {
        com.sos.scheduler.engine.cplusplus.runtime.CppProxy$.MODULE$.requireCppThread();
        try {
            java.lang.String result = job_path__native(cppReference());
            checkIsNotReleased(java.lang.String.class, result);
            return result;
        }
        catch (Exception x) { throw com.sos.scheduler.engine.cplusplus.runtime.CppProxies.propagateCppException(x, this); }
    }

    private static native java.lang.String job_path__native(long cppReference);


    @Override public com.sos.scheduler.engine.kernel.cppproxy.Prefix_logC log() {
        com.sos.scheduler.engine.cplusplus.runtime.CppProxy$.MODULE$.requireCppThread();
        try {
            com.sos.scheduler.engine.kernel.cppproxy.Prefix_logC result = log__native(cppReference());
            checkIsNotReleased(com.sos.scheduler.engine.kernel.cppproxy.Prefix_logC.class, result);
            return result;
        }
        catch (Exception x) { throw com.sos.scheduler.engine.cplusplus.runtime.CppProxies.propagateCppException(x, this); }
    }

    private static native com.sos.scheduler.engine.kernel.cppproxy.Prefix_logC log__native(long cppReference);


    @Override public java.lang.String log_string() {
        com.sos.scheduler.engine.cplusplus.runtime.CppProxy$.MODULE$.requireCppThread();
        try {
            java.lang.String result = log_string__native(cppReference());
            checkIsNotReleased(java.lang.String.class, result);
            return result;
        }
        catch (Exception x) { throw com.sos.scheduler.engine.cplusplus.runtime.CppProxies.propagateCppException(x, this); }
    }

    private static native java.lang.String log_string__native(long cppReference);


    @Override public java.lang.String node_key_string() {
        com.sos.scheduler.engine.cplusplus.runtime.CppProxy$.MODULE$.requireCppThread();
        try {
            java.lang.String result = node_key_string__native(cppReference());
            checkIsNotReleased(java.lang.String.class, result);
            return result;
        }
        catch (Exception x) { throw com.sos.scheduler.engine.cplusplus.runtime.CppProxies.propagateCppException(x, this); }
    }

    private static native java.lang.String node_key_string__native(long cppReference);


    @Override public com.sos.scheduler.engine.kernel.cppproxy.OrderC order() {
        com.sos.scheduler.engine.cplusplus.runtime.CppProxy$.MODULE$.requireCppThread();
        try {
            com.sos.scheduler.engine.kernel.cppproxy.OrderC result = order__native(cppReference());
            checkIsNotReleased(com.sos.scheduler.engine.kernel.cppproxy.OrderC.class, result);
            return result;
        }
        catch (Exception x) { throw com.sos.scheduler.engine.cplusplus.runtime.CppProxies.propagateCppException(x, this); }
    }

    private static native com.sos.scheduler.engine.kernel.cppproxy.OrderC order__native(long cppReference);


    @Override public com.sos.scheduler.engine.kernel.cppproxy.Variable_setC params() {
        com.sos.scheduler.engine.cplusplus.runtime.CppProxy$.MODULE$.requireCppThread();
        try {
            com.sos.scheduler.engine.kernel.cppproxy.Variable_setC result = params__native(cppReference());
            checkIsNotReleased(com.sos.scheduler.engine.kernel.cppproxy.Variable_setC.class, result);
            return result;
        }
        catch (Exception x) { throw com.sos.scheduler.engine.cplusplus.runtime.CppProxies.propagateCppException(x, this); }
    }

    private static native com.sos.scheduler.engine.kernel.cppproxy.Variable_setC params__native(long cppReference);


    @Override public int pid() {
        com.sos.scheduler.engine.cplusplus.runtime.CppProxy$.MODULE$.requireCppThread();
        try {
            return pid__native(cppReference());
        }
        catch (Exception x) { throw com.sos.scheduler.engine.cplusplus.runtime.CppProxies.propagateCppException(x, this); }
    }

    private static native int pid__native(long cppReference);


    @Override public long processStartedAt() {
        com.sos.scheduler.engine.cplusplus.runtime.CppProxy$.MODULE$.requireCppThread();
        try {
            return processStartedAt__native(cppReference());
        }
        catch (Exception x) { throw com.sos.scheduler.engine.cplusplus.runtime.CppProxies.propagateCppException(x, this); }
    }

    private static native long processStartedAt__native(long cppReference);


    @Override public com.sos.scheduler.engine.kernel.cppproxy.Process_classC process_class_or_null() {
        com.sos.scheduler.engine.cplusplus.runtime.CppProxy$.MODULE$.requireCppThread();
        try {
            com.sos.scheduler.engine.kernel.cppproxy.Process_classC result = process_class_or_null__native(cppReference());
            checkIsNotReleased(com.sos.scheduler.engine.kernel.cppproxy.Process_classC.class, result);
            return result;
        }
        catch (Exception x) { throw com.sos.scheduler.engine.cplusplus.runtime.CppProxies.propagateCppException(x, this); }
    }

    private static native com.sos.scheduler.engine.kernel.cppproxy.Process_classC process_class_or_null__native(long cppReference);


    @Override public java.lang.String remote_scheduler_address() {
        com.sos.scheduler.engine.cplusplus.runtime.CppProxy$.MODULE$.requireCppThread();
        try {
            java.lang.String result = remote_scheduler_address__native(cppReference());
            checkIsNotReleased(java.lang.String.class, result);
            return result;
        }
        catch (Exception x) { throw com.sos.scheduler.engine.cplusplus.runtime.CppProxies.propagateCppException(x, this); }
    }

    private static native java.lang.String remote_scheduler_address__native(long cppReference);


    @Override public long running_since_millis() {
        com.sos.scheduler.engine.cplusplus.runtime.CppProxy$.MODULE$.requireCppThread();
        try {
            return running_since_millis__native(cppReference());
        }
        catch (Exception x) { throw com.sos.scheduler.engine.cplusplus.runtime.CppProxies.propagateCppException(x, this); }
    }

    private static native long running_since_millis__native(long cppReference);


    @Override public java.lang.String state_name() {
        com.sos.scheduler.engine.cplusplus.runtime.CppProxy$.MODULE$.requireCppThread();
        try {
            java.lang.String result = state_name__native(cppReference());
            checkIsNotReleased(java.lang.String.class, result);
            return result;
        }
        catch (Exception x) { throw com.sos.scheduler.engine.cplusplus.runtime.CppProxies.propagateCppException(x, this); }
    }

    private static native java.lang.String state_name__native(long cppReference);


    @Override public java.lang.String stderr_path() {
        com.sos.scheduler.engine.cplusplus.runtime.CppProxy$.MODULE$.requireCppThread();
        try {
            java.lang.String result = stderr_path__native(cppReference());
            checkIsNotReleased(java.lang.String.class, result);
            return result;
        }
        catch (Exception x) { throw com.sos.scheduler.engine.cplusplus.runtime.CppProxies.propagateCppException(x, this); }
    }

    private static native java.lang.String stderr_path__native(long cppReference);


    @Override public java.lang.String stdout_path() {
        com.sos.scheduler.engine.cplusplus.runtime.CppProxy$.MODULE$.requireCppThread();
        try {
            java.lang.String result = stdout_path__native(cppReference());
            checkIsNotReleased(java.lang.String.class, result);
            return result;
        }
        catch (Exception x) { throw com.sos.scheduler.engine.cplusplus.runtime.CppProxies.propagateCppException(x, this); }
    }

    private static native java.lang.String stdout_path__native(long cppReference);


    @Override public long stepOrProcessStartedAt() {
        com.sos.scheduler.engine.cplusplus.runtime.CppProxy$.MODULE$.requireCppThread();
        try {
            return stepOrProcessStartedAt__native(cppReference());
        }
        catch (Exception x) { throw com.sos.scheduler.engine.cplusplus.runtime.CppProxies.propagateCppException(x, this); }
    }

    private static native long stepOrProcessStartedAt__native(long cppReference);


    @Override public int step_count() {
        com.sos.scheduler.engine.cplusplus.runtime.CppProxy$.MODULE$.requireCppThread();
        try {
            return step_count__native(cppReference());
        }
        catch (Exception x) { throw com.sos.scheduler.engine.cplusplus.runtime.CppProxies.propagateCppException(x, this); }
    }

    private static native int step_count__native(long cppReference);


    // </editor-fold>
}
