package com.sos.scheduler.engine.tests.jira.js887;

import com.google.common.base.Charsets;
import com.google.common.io.Files;
import com.sos.scheduler.engine.common.time.Time;
import com.sos.scheduler.engine.data.job.TaskEndedEvent;
import com.sos.scheduler.engine.eventbus.EventHandler;
import com.sos.scheduler.engine.test.SchedulerTest;
import com.sos.scheduler.engine.test.util.CommandBuilder;
import org.junit.AfterClass;
import org.junit.BeforeClass;
import org.junit.Test;

import java.io.File;
import java.io.IOException;

import static junit.framework.Assert.assertTrue;

public class BackupLogfileIT extends SchedulerTest {

    private static final CommandBuilder cmd = new CommandBuilder();
    private static final File tempDir = new File(Files.createTempDir(),"log");
    private static final File tempDirWithDot = new File(Files.createTempDir(),"log.dir");
    private static final Time timeout = Time.of(30);

    @BeforeClass
    public static void setUpBeforeClass() throws Exception {
        tempDir.mkdirs();
        tempDirWithDot.mkdirs();
    }

    @AfterClass
    public static void tearDownAfterClass() throws Exception {
        tempDir.delete();
        tempDirWithDot.delete();
    }

    @Test
    public void test1() throws IOException {
        doTest(tempDir, "scheduler.log", "scheduler-old.log");
    }

    @Test
    public void test2() throws IOException {
        doTest(tempDir, "scheduler_log", "scheduler_log-old");
    }

    @Test
    public void test3() throws IOException {
        doTest(tempDirWithDot, "scheduler.log", "scheduler-old.log");
    }

    @Test
    public void test4() throws IOException {
        doTest(tempDirWithDot,"scheduler_log","scheduler_log-old");
    }

    private void doTest(File logDir, String logfileName, String expectedName) throws IOException {
        File logFile = new File(logDir,logfileName);
        File expectedFile = new File(logDir,expectedName);
        controller().prepare();
        Files.touch(logFile);
        controller().activateScheduler("-log-dir=" + logDir.getAbsolutePath(), "-log=" + logFile.getAbsolutePath());
        controller().scheduler().executeXml(cmd.startJobImmediately("test").getCommand());
        controller().waitForTermination(timeout);
        assertTrue("Backup file " + expectedName + " does not exist in " + logDir.getAbsolutePath(),expectedFile.exists());
        String content = Files.toString(expectedFile, Charsets.UTF_8);
        assertTrue("No content of backupfile is allowed.", content.isEmpty());
    }

    @EventHandler
    public void handleTaskEnded(TaskEndedEvent e) throws InterruptedException {
        controller().terminateScheduler();
    }
}
