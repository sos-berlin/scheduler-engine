package sos.spooler.jobs;

import com.google.common.collect.ImmutableMap;
import com.sos.scheduler.engine.kernel.scripting.JobScriptInstanceAdapter;
import com.sos.scheduler.engine.kernel.util.Lazy;
import sos.spooler.Job_impl;

public class ScriptAdapterJob extends Job_impl {
    private final JobScriptInstanceAdapter adapter;
    private final Lazy<ImmutableMap<String,Object>> bindingsLazy = new Lazy<ImmutableMap<String, Object>>() {
        @Override protected ImmutableMap<String, Object> compute() {
            return ImmutableMap.<String, Object>of(
                    "spooler", spooler,
                    "spooler_task", spooler_task,
                    "spooler_job", spooler_job,
                    "spooler_log", spooler_log);
        }
    };

    //TODO Was passiert, wenn der Scriptcode fehlerhaft ist
    //TODO funktioniert das Scripting auch bei remote jobs?

    public ScriptAdapterJob(String language, String script) throws Exception {
        adapter = new JobScriptInstanceAdapter(language, script, bindingsLazy);
    }

    public final boolean spooler_init() throws Exception {
        return adapter.callInit(super.spooler_init());
    }

    public final void spooler_exit() {
        adapter.callExit();
    }

    public final boolean spooler_open() throws Exception {
        return adapter.callOpen(super.spooler_open());
    }

    public final void spooler_close() throws Exception {
        adapter.callClose();
    }

    public final boolean spooler_process() throws Exception {
        return adapter.callProcess(super.spooler_process());
    }

    public final void spooler_on_error() throws Exception {
        adapter.callOnError();
    }

    public final void spooler_on_success() throws Exception {
        adapter.callOnSuccess();
    }

    public final boolean spooler_task_before() throws Exception {
        return adapter.callTaskBefore();
    }

    public final void spooler_task_after() throws Exception {
        adapter.callTaskAfter();
    }

    public final boolean spooler_process_before() throws Exception {
        return adapter.callProcessBefore();
    }

    public final boolean spooler_process_after(boolean spooler_process_result) throws Exception {
        return adapter.callProcessAfter(spooler_process_result);
    }
}
