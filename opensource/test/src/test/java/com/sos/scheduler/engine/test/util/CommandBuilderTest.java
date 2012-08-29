package com.sos.scheduler.engine.test.util;

import com.google.common.io.Resources;
import com.sos.scheduler.engine.kernel.scheduler.SchedulerConstants;
import org.joda.time.DateTime;
import org.joda.time.format.DateTimeFormatter;
import org.joda.time.format.ISODateTimeFormat;
import org.junit.Test;

import java.io.IOException;
import java.net.URL;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertTrue;

public class CommandBuilderTest {

    private final CommandBuilder util = new CommandBuilder();

    private static final String packageRoot = "com/sos/scheduler/engine/test/util/";
    private static final String expectedBuildCommandAddOrder = packageRoot + "expectedBuildCommandAddOrder.txt";
    private static final String expectedBuildCommandAddOrderWithParams = packageRoot + "expectedBuildCommandAddOrderWithParams.txt";
    private static final String expectedBuildCommandAddOrderWithParams2 = packageRoot + "expectedBuildCommandAddOrderWithParams2.txt";
    private static final String expectedBuildCommandStartJobImmidiately = packageRoot + "expectedBuildCommandStartJobImmidiately.txt";

    @Test
    public void addOrderTest() throws IOException {
        String command = util.addOrder("myJobchain").getCommand();
        URL expectedURL = Resources.getResource(expectedBuildCommandAddOrder);
        String expectedCommand = Resources.toString(expectedURL, SchedulerConstants.defaultEncoding);
        assertEquals(expectedCommand,command);
    }

    @Test
    public void addOrderWithParamsTest() throws IOException {
        String command = util.addOrder("myJobchain").addParam("myParam1", "value1").getCommand();
        URL expectedURL = Resources.getResource(expectedBuildCommandAddOrderWithParams);
        String expectedCommand = Resources.toString(expectedURL, SchedulerConstants.defaultEncoding);
        assertEquals(expectedCommand,command);
    }

    @Test
    public void addOrderWithParams2Test() throws IOException {
        util.addParam("myParam1", "value1");
        util.addParam("myParam2", "value2");
        String command = util.addOrder("myJobchain").getCommand();
        URL expectedURL = Resources.getResource(expectedBuildCommandAddOrderWithParams2);
        String expectedCommand = Resources.toString(expectedURL, SchedulerConstants.defaultEncoding);
        assertEquals(expectedCommand,command);
    }

    @Test
    public void showCalendarTest() throws IOException {
        int duration = 60;
        String command = util.showCalendar(duration, What.orders).getCommand();
        DateTime begin = util.getLastBegin();
        DateTime end = util.getLastEnd();
        DateTimeFormatter fmt = ISODateTimeFormat.dateHourMinuteSecond();
        String expectedFile = "<show_calendar before='" + fmt.print(end) + "' from='" + fmt.print(begin) + "' limit='10' what='orders'></show_calendar>";
        assertTrue("expected value is '" + expectedFile + "'",expectedFile.equals(command));
    }

    @Test
    public void startJobImmidiatelyTest() throws IOException {
        util.addParam("myParam1", "value1");
        util.addParam("myParam2", "value2");
        String command = util.startJobImmediately("myJob").getCommand();
        URL expectedURL = Resources.getResource(expectedBuildCommandStartJobImmidiately);
        String expectedCommand = Resources.toString(expectedURL, SchedulerConstants.defaultEncoding);
        assertEquals(expectedCommand,command);
    }

}
