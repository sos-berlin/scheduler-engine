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

import com.sos.scheduler.engine.kernel.scripting.APIModuleInstance;
import sos.spooler.Job_impl;
import sos.spooler.Monitor_impl;
import sos.spooler.Variable_set;

import java.util.Map;

/**
 * Created by IntelliJ IDEA.
 * User: ss
 * Date: 07.02.12
 * Time: 16:27
 */
public class ScriptAdapterJob extends Job_impl {

    private APIModuleInstance scriptModule = null;

    //TODO Was passiert, wenn der Scriptcode fehlerhaft ist
    //TODO Was passiert, wenn der Scriptcode aus einer Datei geladen wird
    //TODO was passiert bei gemischtem Sourceocode (z.B. shell_script, Monitore in javax);
    //TODO funktioniert das Scripting auch bei remote jobs?
    //TODO prüfen, ob die Scheduler Parameter mit SCHEDULER__ statt mit SCHEDULER_ anfangen sollten

    private boolean bindingsSet = false;

    public ScriptAdapterJob() {
        System.err.println("empty konstruktor");
    }

    public ScriptAdapterJob(int x) {
        System.err.println("empty konstruktor " + x);
    }

    public ScriptAdapterJob(String language, String code) {

        System.err.println("language=" + language);
        System.err.println("code=" + code);
        try {
            scriptModule = new APIModuleInstance(language, code);
        } catch (Exception e) {
            System.err.println(e.getMessage());
            throw new RuntimeException(e);
        }
    }

    public boolean spooler_init() throws Exception {
        try {
            setBindings();
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
            setBindings();
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
        if (scriptModule != null) {
            spooler_log.info("call function " + function);
            scriptModule.call(function);
        }
    }

    private boolean callMethod(String function, boolean defaultResult) {
        boolean result = defaultResult;
        if (scriptModule != null) {
            spooler_log.info("call function " + function);
            Object resultValue = scriptModule.call(function);
            if (resultValue != null)
                result = (Boolean) resultValue;
        } else
            spooler_log.info("script module not set");
        spooler_log.info("result is " + result);
        return result;
    }

    private void prepareEnvironment() {
        if (scriptModule == null) {
            String test = System.getProperty("test");
            Variable_set v = spooler_task.params();
            String language = v.var("SCHEDULER_SCRIPT_LANGUAGE");
            String code = v.var("SCHEDULER_SCRIPT_CODE");
            // String adapter = v.var("SCHEDULER_SCRIPT_ADAPTER_JOB");
            spooler_log.info("language=" + language);
            spooler_log.info("code=" + code);
            spooler_log.info("test=" + code);
            // spooler_log.info("adapter=" + adapter);
            //
            Map<String, String> env = System.getenv();
            for (String envName : env.keySet()) {
                if (envName.startsWith("SCHEDULER_")) spooler_log.info(envName + "=" + env.get(envName));
            }

            try {
                scriptModule = new APIModuleInstance(language, code);
                setBindings();
            } catch (Exception e) {
                spooler_log.error(e.getMessage());
                throw new RuntimeException(e);
            }
        }
    }

    private void setBindings() {
        if (!bindingsSet) {
            scriptModule.addObject(spooler, "spooler");
            scriptModule.addObject(spooler_task, "spooler_task");
            scriptModule.addObject(spooler_job, "spooler_job");
            scriptModule.addObject(spooler_log, "spooler_log");
            bindingsSet = true;
        }
    }
}
