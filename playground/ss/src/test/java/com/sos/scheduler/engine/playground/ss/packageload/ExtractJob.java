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
