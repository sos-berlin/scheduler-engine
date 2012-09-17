package com.sos.scheduler.engine.playground.ss.processlist;

import com.sos.scheduler.engine.kernel.scheduler.SchedulerException;
import org.apache.log4j.Logger;
import sos.spooler.Job_impl;

import java.io.BufferedReader;
import java.io.InputStreamReader;

public class ShowProcessesJob extends Job_impl {

    private static final Logger logger			= Logger.getLogger(ShowProcessesJob.class);

    @SuppressWarnings("unused")
    private	final String		conSVNVersion			= "$Id: ReadParameterJob.java 16850 2012-03-21 12:14:23Z ss $";

    public ShowProcessesJob() {
        logger.info("creating instance of class");
    }

    @Override
    public boolean spooler_init() {
        logger.info("calling spooler_init");
        return true;
    }

    @Override
    public boolean spooler_open() {
        logger.info("calling spooler_open");
        return true;
    }

    @Override
    public boolean spooler_process() throws Exception {

        try {
            super.spooler_process();

            try {
                Process p = Runtime.getRuntime().exec(System.getenv("windir") +"\\system32\\"+"tasklist.exe");
                BufferedReader input = new BufferedReader(new InputStreamReader(p.getInputStream()));
                logger.info("list of all running java processes ...");
                String line = input.readLine();
                logger.info(line);
                line = input.readLine();
                logger.info(line);
                while ((line = input.readLine()) != null) {
                    if (line.startsWith("java.exe")) logger.info(line); //<-- Parse data here.
                }
                input.close();
            } catch (Exception err) {
                err.printStackTrace();
            }
        }
        catch (Exception e) {
            throw new SchedulerException("error in job",e);
        }
        spooler_task.end();
        return true;

    } // spooler_process

}
