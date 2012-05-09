/*
 * <div class="sos_branding">
 *   <p>(c) 2012 SOS GmbH - Berlin (<a style='color:silver' href='http://www.sos-berlin.com'>http://www.sos-berlin.com</a>)</p>
 * </div>
 */

package com.sos.scheduler.engine.tests.jira.js628;

import sos.spooler.Job_impl;

public class SpoolerProcessFalse extends Job_impl {
    @Override
    public boolean spooler_process() {
        return false;
    }
}
