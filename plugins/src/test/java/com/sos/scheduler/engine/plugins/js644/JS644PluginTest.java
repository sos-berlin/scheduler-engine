package com.sos.scheduler.engine.plugins.js644;

import static com.sos.scheduler.engine.plugins.js644.JS644PluginTest.M.jobActivated;
import static com.sos.scheduler.engine.plugins.js644.JS644PluginTest.M.jobchainActivated;
import static org.hamcrest.MatcherAssert.assertThat;

import java.io.File;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.OutputStream;

import org.apache.log4j.Logger;
import org.junit.Test;

import com.sos.scheduler.engine.eventbus.HotEventHandler;
import com.sos.scheduler.engine.kernel.folder.AbsolutePath;
import com.sos.scheduler.engine.kernel.folder.event.FileBasedActivatedEvent;
import com.sos.scheduler.engine.kernel.job.Job;
import com.sos.scheduler.engine.main.SchedulerState;
import com.sos.scheduler.engine.test.SchedulerTest;
import com.sos.scheduler.engine.kernel.util.Time;
import com.sos.scheduler.engine.kernel.util.sync.Gate;

public class JS644PluginTest extends SchedulerTest {
    private static final Logger logger = Logger.getLogger(JS644PluginTest.class);
    private static final AbsolutePath jobPath = new AbsolutePath("/A");
    private static final AbsolutePath jobchainPath = new AbsolutePath("/a");
    private static final Time timeout = shortTimeout;

    enum M { jobActivated, jobchainActivated }
    private final Gate<M> gate = new Gate<M>();
    private volatile boolean schedulerIsActive = false;

    @Test public void test() throws Exception {
        controller().startScheduler();
        controller().waitUntilSchedulerState(SchedulerState.active);
        schedulerIsActive = true;
        modifyJobFile();
        gate.expect(jobActivated, timeout);
        gate.expect(jobchainActivated, timeout);
    }

    private void modifyJobFile() throws IOException {
        File jobFile = fileBasedFile(scheduler().getJobSubsystem().job(jobPath));
        assertThat(jobFile + " does not exist", jobFile.exists());
        OutputStream out = new FileOutputStream(jobFile, true);
        try {
            out.write(' ');
        } finally {
            out.close();
        }
    }

    @HotEventHandler public void handleEvent(FileBasedActivatedEvent e) throws InterruptedException {
        if (schedulerIsActive) {
            if (e.getObject() == scheduler().getJobSubsystem().job(jobPath))
                gate.put(jobActivated);
            else
            if (e.getObject() == scheduler().getOrderSubsystem().jobChain(jobchainPath))
                gate.put(jobchainActivated);
        }
    }

    private File fileBasedFile(Job o) {
        return new File(controller().environment().configDirectory(), o.getPath() + ".job.xml");
    }
}
