/*
 * <div class="sos_branding">
 *   <p>(c) 2012 SOS GmbH - Berlin (<a style='color:silver' href='http://www.sos-berlin.com'>http://www.sos-berlin.com</a>)</p>
 * </div>
 */

package com.sos.scheduler.engine.test.jobs;

import com.sos.JSHelper.Exceptions.JobSchedulerException;
import sos.spooler.Job_impl;

public class SleepOrderJob extends Job_impl {

    private static final String blank = "";
    private static final String parameterName = "RUNTIME_IN_SECONDS";
    private static final long defaultRuntime = 5;
    
    @Override public boolean spooler_process() {
        String param = spooler_task.order().params().value(parameterName);
        long runtime = ((param.equals(blank)) ?  defaultRuntime : Integer.parseInt(param)) * 1000;
        try {
            Thread.sleep( runtime );
        } catch (InterruptedException e) {
            throw new JobSchedulerException("error processing Thread.sleep(" + runtime + ")",e);
        }
        return true;
    }
}
