/*
 * <div class="sos_branding">
 *   <p>(c) 2012 SOS GmbH - Berlin (<a style='color:silver' href='http://www.sos-berlin.com'>http://www.sos-berlin.com</a>)</p>
 * </div>
 */

package com.sos.scheduler.engine.playground.zschimmer.plugin.watchdog;

import sos.spooler.Job_impl;

public class JobDelayTask extends Job_impl {
    @Override
    public boolean spooler_process() {
        spooler_task.set_delay_spooler_process(1);
        return true;
    }
}
