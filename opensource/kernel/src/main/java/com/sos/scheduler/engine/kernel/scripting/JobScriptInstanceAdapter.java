package com.sos.scheduler.engine.kernel.scripting;

import com.google.common.collect.ImmutableMap;
import com.sos.scheduler.engine.kernel.util.Lazy;

public class JobScriptInstanceAdapter {
    private final ScriptInstance scriptInstance;

    //TODO Was passiert, wenn der Scriptcode fehlerhaft ist
    //TODO funktioniert das Scripting auch bei remote jobs?

    public JobScriptInstanceAdapter(String language, Lazy<ImmutableMap<String,Object>> bindingsLazy, String script) {
        this.scriptInstance = new ScriptInstance(language, bindingsLazy, script);
    }

    public final boolean callInit(boolean deflt) throws Exception {
        loadScript();
        return scriptInstance.callBooleanWhenExists("spooler_init", deflt);
    }

    public final void callExit() {
        try {
            scriptInstance.callWhenExists("spooler_exit");
        } finally {
            scriptInstance.close();
        }
    }

    public final boolean callOpen(boolean deflt) throws Exception {
        return scriptInstance.callBooleanWhenExists("spooler_open", deflt);
    }

    public final void callClose() throws Exception {
        scriptInstance.callWhenExists("spooler_close");
    }

    public final boolean callProcess(boolean deflt) throws Exception {
        return scriptInstance.callBooleanWhenExists("spooler_process", deflt);
    }

    public final void callOnError() throws Exception {
        scriptInstance.callWhenExists("spooler_on_error");
    }

    public final void callOnSuccess() throws Exception {
        scriptInstance.callWhenExists("spooler_on_success");
    }

    public final boolean callTaskBefore() throws Exception {
        loadScript();
        return scriptInstance.callBooleanWhenExists("spooler_task_before", true);
    }

    public final void callTaskAfter() throws Exception {
        try {
            scriptInstance.callWhenExists("spooler_task_after");
        } finally {
            scriptInstance.close();
        }
    }

    public final boolean callProcessBefore() throws Exception {
        return scriptInstance.callBooleanWhenExists("spooler_process_before", true);
    }

    public final boolean callProcessAfter(boolean spoolerProcessResult) throws Exception {
        return scriptInstance.callBooleanWhenExists("spooler_process_after", spoolerProcessResult);
    }

    private void loadScript() {
        scriptInstance.loadScript();
    }
}
