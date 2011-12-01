package com.sos.scheduler.engine.tests.jira.js707;

import static com.google.common.collect.Maps.newHashMap;
import static java.lang.Thread.sleep;
import static org.hamcrest.MatcherAssert.assertThat;
import static org.hamcrest.Matchers.equalTo;

import java.io.BufferedReader;
import java.io.IOException;
import java.io.InputStream;
import java.io.InputStreamReader;
import java.util.Map;

import org.apache.log4j.Logger;
import org.junit.Ignore;
import org.junit.Test;

import com.sos.scheduler.engine.main.CppBinary;
import com.sos.scheduler.engine.test.SchedulerTest;

public class JS707Test extends SchedulerTest {
    private static final Logger logger = Logger.getLogger(JS707Test.class);
    private static final String schedulerId = "testScheduler";
    private static final int portNumber = 47523;
    private static final int backupPortNumber = portNumber + 1;
    private static final int databasePortNumber = portNumber + 2;
    private final SchedulerH2DatabaseServer database = SchedulerH2DatabaseServer.newTcpServer(databasePortNumber);

    @Ignore     // Test funktioniert noch nicht.
    @Test public void test() throws Exception {
        database.start();
        controller().activateScheduler("-e", "-id="+schedulerId, "-exclusive", "-tcp-port="+portNumber, "-db="+database.hostwarePath());
        Process backupScheduler = startBackupScheduler();
        Thread stdoutThread = new Thread(new StreamLogger(backupScheduler.getInputStream(), "backup scheduler stdout"));
        Thread stderrThread = new Thread(new StreamLogger(backupScheduler.getErrorStream(), "backup scheduler stderr"));
        stdoutThread.start();
        stderrThread.start();
        try {
            sleep(10*1000);
            scheduler().executeXml("<terminate timeout='0'/>");
            int exitCode = backupScheduler.waitFor();
            assertThat(exitCode, equalTo(0));
        }
        finally {
            backupScheduler.destroy();
            backupScheduler.waitFor();
            stdoutThread.join();
            stderrThread.join();
            database.stop();
        }
    }

    private Process startBackupScheduler() throws IOException {
        String[] args = {
                controller().cppBinaries().file(CppBinary.exeFilename).getPath(),
                "-sos.ini="+ controller().environment().sosIniFile(),
                "-ini="+ controller().environment().iniFile(),
                "-id="+ schedulerId,
                "-tcp-port="+backupPortNumber,
                "-db="+database.hostwarePath(),
                "-exclusive",
                "-backup",
                "-e",
                controller().environment().configDirectory().toString()};
        Map<String,String> env = newHashMap(System.getenv());
        env.put("SCHEDULER_TEST_JAVA_CLASSPATH", System.getProperty("java.class.path"));
        return Runtime.getRuntime().exec(args, envArrayFromMap(env));
    }

    private static String[] envArrayFromMap(Map<String,String> map) {
        String[] result = new String[map.size()];
        int i =  0;
        for (Map.Entry<String,String> e: map.entrySet())
            result[i++] = e.getKey() +"="+ e.getValue();
        return result;
    }

    private static class StreamLogger implements Runnable {
        private final BufferedReader reader;
        private final String prefix;

        StreamLogger(InputStream input, String prefix) {
            this.prefix = prefix;
            this.reader = new BufferedReader(new InputStreamReader(input));
        }

        @Override public void run() {
            try {
                while(true) {
                    String line = reader.readLine();
                    if (line == null) break;
                    logger.info(prefix+"> "+line);
                }
            } catch (IOException e) {
                throw new RuntimeException(e);
            }
        }
    }
}
