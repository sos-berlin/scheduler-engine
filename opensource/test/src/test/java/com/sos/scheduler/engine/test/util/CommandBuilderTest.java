package com.sos.scheduler.engine.test.util;

import com.google.common.io.Files;
import com.sos.scheduler.engine.kernel.scheduler.SchedulerConstants;
import org.apache.log4j.Logger;
import org.joda.time.DateTime;
import org.joda.time.format.DateTimeFormatter;
import org.joda.time.format.ISODateTimeFormat;
import org.junit.Test;

import java.io.File;
import java.io.FileNotFoundException;
import java.io.IOException;

import static org.junit.Assert.assertTrue;

public class CommandBuilderTest {
	
	private static final Logger logger = Logger.getLogger(CommandBuilderTest.class);
	private final CommandBuilder util = new CommandBuilder();

    private static final String packageRoot = "com/sos/scheduler/engine/test/util/";
    private static final String expectedBuildCommandAddOrder = packageRoot + "expectedBuildCommandAddOrder.txt";
	private static final String expectedBuildCommandAddOrderWithParams = packageRoot + "expectedBuildCommandAddOrderWithParams.txt";
	private static final String expectedBuildCommandAddOrderWithParams2 = packageRoot + "expectedBuildCommandAddOrderWithParams2.txt";
	private static final String expectedBuildCommandStartJobImmidiately = packageRoot + "expectedBuildCommandStartJobImmidiately.txt";
    
	@Test
	public void addOrderTest() throws IOException {
		String command = util.addOrder("myJobchain").getCommand();
        File file = File.createTempFile("testBuildCommandAddOrder",".txt");
		Files.append(command, file, SchedulerConstants.defaultEncoding);
		File expectedFile = FileUtils.getResourceFile(expectedBuildCommandAddOrder);
		assertEqualFiles(file, expectedFile);
        file.delete();
	}

	@Test
	public void addOrderWithParamsTest() throws IOException {
		String command = util.addOrder("myJobchain").addParam("myParam1", "value1").getCommand();
		File file = File.createTempFile("testBuildCommandAddOrderWithParam",".txt");
		Files.append(command, file, SchedulerConstants.defaultEncoding);
		File expectedFile = FileUtils.getResourceFile(expectedBuildCommandAddOrderWithParams);
		assertEqualFiles(file, expectedFile);
        file.delete();
	}

	@Test
	public void addOrderWithParams2Test() throws IOException {
		util.addParam("myParam1", "value1");
		util.addParam("myParam2", "value2");
		String command = util.addOrder("myJobchain").getCommand();
		File file = File.createTempFile("testBuildCommandAddOrderWithParam2",".txt");
		Files.append(command, file, SchedulerConstants.defaultEncoding);
		File expectedFile = FileUtils.getResourceFile(expectedBuildCommandAddOrderWithParams2);
		assertEqualFiles(file, expectedFile);
        file.delete();
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
		File file = File.createTempFile("testBuildCommandStartJobImmidiately",".txt");
		Files.append(command, file, SchedulerConstants.defaultEncoding);
		File expectedFile = FileUtils.getResourceFile(expectedBuildCommandStartJobImmidiately);
		assertEqualFiles(file, expectedFile);
        file.delete();
	}
	
	private void assertEqualFiles(File resultFile, File expectedFile) throws IOException {
        if (!resultFile.exists()) throw new FileNotFoundException(resultFile.getAbsolutePath());
        if (!expectedFile.exists()) throw new FileNotFoundException(expectedFile.getAbsolutePath());
		String expectedContent = Files.toString(expectedFile, SchedulerConstants.defaultEncoding);
		logger.debug("expected content: " + expectedContent);
		assertTrue("content of file '" + resultFile.getAbsolutePath() + "' is not estimated",Files.equal(resultFile, expectedFile));
	}
	
}
