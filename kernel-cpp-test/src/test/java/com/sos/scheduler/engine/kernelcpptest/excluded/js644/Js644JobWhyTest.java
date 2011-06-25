package com.sos.scheduler.engine.kernelcpptest.excluded.js644;

import com.sos.scheduler.engine.kernel.test.SchedulerTest;
import org.apache.log4j.Logger;
import org.junit.Test;
import static com.sos.scheduler.engine.kernelcpptest.excluded.js644.Configuration.*;


public class Js644JobWhyTest extends SchedulerTest {
    private static final Logger logger = Logger.getLogger(Js644JobWhyTest.class);


    public Js644JobWhyTest() {
        super(configFilenames);
    }


    @Test public void test() throws Exception {
        startScheduler("-e");
        getScheduler().executeXml("<modify_job job='" + jobPaths.get(1) + "' cmd='stop'/>");
        getScheduler().executeXml("<add_order job_chain='" + jobchainName + "' id='1'/>");
        getScheduler().executeXml("<add_order job_chain='" + jobchainName + "' id='2'/>");
        getScheduler().executeXml("<add_order job_chain='" + jobchainName + "' id='3'/>");
        getScheduler().executeXml("<job_chain.modify job_chain='" + jobchainName + "' state='stopped'/>");
        getScheduler().executeXml("<job_chain_node.modify job_chain='" + jobchainName + "' state='200' action='next_state'/>");
        for (String j: jobPaths) logger.info(getScheduler().executeXml("<job.why job='" + j + "'/>"));    //TODO Test fehlt
    }
}
