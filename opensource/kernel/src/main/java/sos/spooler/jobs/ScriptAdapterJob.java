package sos.spooler.jobs;

import com.google.common.collect.ImmutableMap;
import com.sos.scheduler.engine.kernel.scripting.JobScriptInstanceAdapter;
import com.sos.scheduler.engine.kernel.util.Lazy;
import sos.spooler.Job_impl;

import static sos.spooler.Beans.toBean;

public class ScriptAdapterJob extends Job_impl {
    private final JobScriptInstanceAdapter adapter;

    //TODO Was passiert, wenn der Scriptcode fehlerhaft ist
    //TODO funktioniert das Scripting auch bei remote jobs? sos.spooler und com.sos.scheduler.engine.scripting in eigenes Artefakt auslagern, das von Taskprozessen verwendet wird -> com.sos.scheduler.task

    public ScriptAdapterJob(String language, String script) throws Exception {
        adapter = new JobScriptInstanceAdapter(
                language,
                new Lazy<ImmutableMap<String,Object>>() {
                    @Override protected ImmutableMap<String,Object> compute() {
                        return ImmutableMap.<String,Object>of(
                                "spooler", toBean(spooler),
                                "spooler_task", toBean(spooler_task),
                                "spooler_job", toBean(spooler_job),
                                "spooler_log", toBean(spooler_log));
                    }},
                script);
    }

    @Override public final boolean spooler_init() throws Exception {
        return adapter.callInit(super.spooler_init());
    }

    @Override public final void spooler_exit() {
        adapter.callExit();
    }

    @Override public final boolean spooler_open() throws Exception {
        return adapter.callOpen(super.spooler_open());
    }

    @Override public final void spooler_close() throws Exception {
        adapter.callClose();
    }

    @Override public final boolean spooler_process() throws Exception {
        return adapter.callProcess(super.spooler_process());
    }

    @Override public final void spooler_on_error() throws Exception {
        adapter.callOnError();
    }

    @Override public final void spooler_on_success() throws Exception {
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

    public final boolean spooler_process_after(boolean spoolerProcessResult) throws Exception {
        return adapter.callProcessAfter(spoolerProcessResult);
    }
}
