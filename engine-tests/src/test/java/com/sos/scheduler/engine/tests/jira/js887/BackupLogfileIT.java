package com.sos.scheduler.engine.tests.jira.js887;

import com.google.common.base.Charsets;
import com.google.common.io.Files;
import com.sos.scheduler.engine.data.event.KeyedEvent;
import com.sos.scheduler.engine.data.job.TaskEnded;
import com.sos.scheduler.engine.eventbus.EventHandler;
import com.sos.scheduler.engine.test.SchedulerTest;
import com.sos.scheduler.engine.test.util.CommandBuilder;
import java.io.File;
import java.io.IOException;
import java.time.Duration;
import org.junit.AfterClass;
import org.junit.BeforeClass;
import org.junit.Test;
import static com.sos.scheduler.engine.common.system.Files.tryRemoveDirectoryRecursivly;
import static java.util.Arrays.asList;
import static org.junit.Assert.assertTrue;

public class BackupLogfileIT extends SchedulerTest {

    private static final Duration timeout = Duration.ofSeconds(30);
    private static File testDirectory;
    private static File tempDirWithoutDot;
    private static File tempDirWithDot;

    private final CommandBuilder cmd = new CommandBuilder();

    @BeforeClass
    public static void setUpBeforeClass() throws Exception {
        testDirectory = Files.createTempDir();
        tempDirWithoutDot = new File(testDirectory, "log");
        tempDirWithDot = new File(testDirectory, "log.dir");
        tempDirWithoutDot.mkdirs();
        tempDirWithDot.mkdirs();
    }

    @AfterClass
    public static void tearDownAfterClass() throws Exception {
        tryRemoveDirectoryRecursivly(testDirectory);
    }

    @Test
    public void test1() throws IOException {
        doTest(tempDirWithoutDot, "scheduler.log", "scheduler-old.log");
    }

    @Test
    public void test2() throws IOException {
        doTest(tempDirWithoutDot, "scheduler_log", "scheduler_log-old");
    }

    @Test
    public void test3() throws IOException {
        doTest(tempDirWithDot, "scheduler.log", "scheduler-old.log");
    }

    @Test
    public void test4() throws IOException {
        doTest(tempDirWithDot, "scheduler_log", "scheduler_log-old");
    }

    private void doTest(File logDir, String logfileName, String expectedName) throws IOException {
        File logFile = new File(logDir, logfileName);
        File expectedFile = new File(logDir, expectedName);
        controller().prepare();
        Files.touch(logFile);
        controller().activateScheduler(asList("-log-dir=" + logDir.getAbsolutePath(), "-log=" + logFile.getAbsolutePath()));
        controller().scheduler().executeXml(cmd.startJobImmediately("test").getCommand());
        controller().waitForTermination(timeout);
        assertTrue("Backup file " + expectedName + " does not exist in " + logDir.getAbsolutePath(), expectedFile.exists());
        String content = Files.toString(expectedFile, Charsets.UTF_8);
        assertTrue("No content of backupfile is allowed.", content.isEmpty());
    }

    @EventHandler
    public void handleEvent(KeyedEvent<TaskEnded> e) {
        controller().terminateScheduler();
    }
}
