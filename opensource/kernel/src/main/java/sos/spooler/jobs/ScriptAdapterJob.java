/*
 * <div class="sos_branding">
 *   <p>(c) 2012 SOS GmbH - Berlin (<a style='color:silver' href='http://www.sos-berlin.com'>http://www.sos-berlin.com</a>)</p>
 * </div>
 */

/*
 * <div class="sos_branding">
 *   <p>(c) 2012 SOS GmbH - Berlin (<a style='color:silver' href='http://www.sos-berlin.com'>http://www.sos-berlin.com</a>)</p>
 * </div>
 */

package sos.spooler.jobs;

import com.sos.scheduler.engine.kernel.scheduler.SchedulerException;
import com.sos.scheduler.engine.kernel.scripting.APIModuleInstance;
import sos.spooler.Job_impl;

/**
 * Created by IntelliJ IDEA.
 * User: ss
 * Date: 07.02.12
 * Time: 16:27
 */
public class ScriptAdapterJob extends Job_impl {

    private final APIModuleInstance scriptModule;
    private final String language;
    private final String code;

    //TODO Was passiert, wenn der Scriptcode fehlerhaft ist
    //TODO funktioniert das Scripting auch bei remote jobs?

    private boolean bindingsSet = false;

    public ScriptAdapterJob(String language, String code) {

        this.language = language;
        this.code = code;
        try {
            this.scriptModule = new APIModuleInstance(language, code);
        } catch (Exception e) {
            throw new SchedulerException("error creating instance of APIModuleInstance",e);
        }
    }

    public boolean spooler_init() throws Exception {
        String method = "spooler_init";
        try {
            setBindings();
            callMethod(method);
        } catch (Exception e) {
            spooler_log.error(e.getMessage());
            throw new SchedulerException(method + " failed",e);
        }
        return true;
    }

    public void spooler_exit() throws Exception {
        callMethod("spooler_exit");
    }

    public boolean spooler_open() throws Exception {
        return callMethod("spooler_open", super.spooler_open());
    }

    public void spooler_close() throws Exception {
        callMethod("spooler_close");
    }

    public boolean spooler_process() throws Exception {
        return callMethod("spooler_process", super.spooler_process());
    }

    public void spooler_on_error() throws Exception {
        callMethod("spooler_on_error");
    }

    public void spooler_on_success() throws Exception {
        callMethod("spooler_on_success");
    }

    public boolean spooler_task_before() throws Exception {
        String method = "spooler_task_before";
        try {
            setBindings();
            callMethod(method);
        } catch (Exception e) {
            spooler_log.error(e.getMessage());
            throw new SchedulerException(method + " failed",e);
        }
        return true;
    }


    public void spooler_task_after() throws Exception {
        callMethod("spooler_task_after");
    }


    public boolean spooler_process_before() throws Exception {
        return callMethod("spooler_process_before", true);
    }


    public boolean spooler_process_after(boolean spooler_process_result) throws Exception {
        return callMethod("spooler_process_after", spooler_process_result);
    }

    private void callMethod(String function) {
        scriptModule.call(function);
    }

    private boolean callMethod(String function, boolean defaultResult) {
        return scriptModule.callBoolean(function,defaultResult);
    }

    private void setBindings() {
        if (!bindingsSet) {
            scriptModule.addObject(spooler, "spooler");
            scriptModule.addObject(spooler_task, "spooler_task");
            scriptModule.addObject(spooler_job, "spooler_job");
            scriptModule.addObject(spooler_log, "spooler_log");
            scriptModule.addObject(spooler_log, "logger");
            bindingsSet = true;
        }
    }
}
