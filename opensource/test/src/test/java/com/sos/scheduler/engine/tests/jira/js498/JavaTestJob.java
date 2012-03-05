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

/*
 * <div class="sos_branding">
 *   <p>(c) 2012 SOS GmbH - Berlin (<a style='color:silver' href='http://www.sos-berlin.com'>http://www.sos-berlin.com</a>)</p>
 * </div>
 */

package com.sos.scheduler.engine.tests.jira.js498;

import com.sos.scheduler.engine.kernel.scripting.APIModuleInstance;
import sos.spooler.Job_impl;
import sos.spooler.Variable_set;

import java.util.Map;

/**
 * Created by IntelliJ IDEA.
 * User: ss
 * Date: 07.02.12
 * Time: 16:27
 */
public class JavaTestJob extends Job_impl {

    private APIModuleInstance scriptModule = null;

    //TODO Was passiert, wenn der Scriptcode fehlerhaft ist
    //TODO Was passiert, wenn der Scriptcode aus einer Datei geladen wird
    //TODO was passiert bei gemischtem Sourceocode (z.B. shell_script, Monitore in javax);
    //TODO funktioniert das Scripting auch bei remote jobs?
    //TODO prüfen, ob die Scheduler Parameter mit SCHEDULER__ statt mit SCHEDULER_ anfangen sollten

    private boolean bindingsSet = false;

    public JavaTestJob() {
        System.err.println("empty konstruktor");
    }

    public boolean spooler_init() throws Exception {
        try {
            callMethod("spooler_init");
        } catch (Exception e) {
            spooler_log.error(e.getMessage());
            throw new RuntimeException(e);
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
        try {
            callMethod("spooler_task_before");
        } catch (Exception e) {
            spooler_log.error(e.getMessage());
            throw new RuntimeException(e);
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
        spooler_log.info("call function " + function);
    }

    private boolean callMethod(String function, boolean defaultResult) {
        boolean result = defaultResult;
        spooler_log.info("result is " + result);
        return result;
    }

}
