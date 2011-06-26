package com.sos.scheduler.engine.kernelcpptest.xmlcommand.job_why;

import com.sos.scheduler.engine.kernel.util.XmlUtils;
import com.google.common.collect.ImmutableMap;
import com.google.common.collect.ImmutableSet;
import com.sos.scheduler.engine.kernel.test.SchedulerTest;
import java.util.Map;
import org.apache.log4j.Logger;
import org.junit.*;
import org.w3c.dom.Element;
import static com.google.common.collect.Sets.difference;
import static com.sos.scheduler.engine.kernel.util.XmlUtils.*;
import static com.sos.scheduler.engine.kernelcpptest.xmlcommand.job_why.Configuration.*;
import static org.hamcrest.MatcherAssert.assertThat;
import static org.hamcrest.Matchers.*;



public class JobWhyTest {
    private static final Logger logger = Logger.getLogger(JobWhyTest.class);

    private static final SchedulerTest schedulerTest = new SchedulerTest(configFilenames) {};
    private static Map<String,Element> results = null;

    
    @BeforeClass public static void beforeClass() {
        schedulerTest.startScheduler();
        results = executeJobWhy();
    }


    private static Map<String,Element> executeJobWhy() {
        ImmutableMap.Builder<String,Element> b = new ImmutableMap.Builder<String,Element>();
        for (String j: jobNames)  b.put(j, executeJobWhy(j));
        return b.build();
    }
    
    
    private static Element executeJobWhy(String jobPath) {
        String xml = schedulerTest.getScheduler().executeXml("<job.why job='" + jobPath + "'/>");
        Element result = elementXPath(loadXml(xml), "/spooler/answer/job");
        logger.debug(jobPath + ": " + toXml(result));
        return result;
    }


    @AfterClass public static void afterClass() throws Throwable {
        schedulerTest.terminateAndCleanUp();
    }


    @Test public void testJobStopped() {
        checkXPathForJobsExactly("obstacle[@state='stopped']", "c");
    }


    @Test public void testJobMinTasks() {
        checkXPathForJobsExactly("start_reason[@min_tasks='1']/obstacle[@in_period='false']", "minTasks");
    }


    @Test public void testJobMaxTasks() {
        checkXPathForJobsExactly("obstacle[@max_tasks='0']", "maxTasks");
    }


    @Test public void testJobchain() {
        for (String jobName: results.keySet()) {
            Element e = results.get(jobName);
            elementXPath(e, jobchainXPath(jobName));
        }
    }

    
    @Test public void testJobchainStopped() {
        checkXPathForAllJobs("job_chain_nodes/job_chain_node/job_chain/obstacle[@state='stopped']");
    }


    private static void checkXPathForAllJobs(String xPath) {
        checkXPathForJobsExactly(xPath, jobNames.toArray(new String[jobNames.size()]));
    }
    

    private static void checkXPathForJobsExactly(String xPath, String... selectedJobNames) {
        for (String jobName: selectedJobNames)
            elementXPath(results.get(jobName), xPath);
        for (String jobName: difference(ImmutableSet.copyOf(selectedJobNames), ImmutableSet.copyOf(jobNames)))
            assertThat(booleanXPath(results.get(jobName), "not(obstacle[@state='stopped'])"), equalTo(false));
    }

    
    private static String jobchainXPath(String jobName) {
        return jobchainNodeXPath(jobName) + "/job_chain[@path='/j']";
    }


    private static String jobchainNodeXPath(String jobName) {
        return "job_chain_nodes/job_chain_node[@order_state='state-" + jobName + "']";
    }
}
