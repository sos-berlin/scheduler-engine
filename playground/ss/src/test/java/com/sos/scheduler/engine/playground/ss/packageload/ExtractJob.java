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

package com.sos.scheduler.engine.playground.ss.packageload;

import sos.spooler.Job_impl;

public class ExtractJob extends Job_impl {

    @Override
    public boolean spooler_process() {
        String classname = spooler_task.order().params().value("classname").replace(".","/") + ".class";
        spooler_log.info("class: " + classname);
        /*
        URL url = ClassLoader.getSystemResource(classname);
        spooler_log.info("URL: " + url.toExternalForm());
        */
        return true;
    }

    private void readResources() {
    
    }
}
