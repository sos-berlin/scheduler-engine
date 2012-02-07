package com.sos.scheduler.engine.test.util;

import static org.junit.Assert.assertTrue;

import java.io.File;
import java.io.FileNotFoundException;
import java.io.IOException;
import java.nio.charset.Charset;

import org.apache.log4j.Logger;
import org.joda.time.DateTime;
import org.joda.time.format.DateTimeFormatter;
import org.joda.time.format.ISODateTimeFormat;
import org.junit.Test;

import com.google.common.io.Files;

public class CommandBuilderTest {
	
	private static final Logger logger = Logger.getLogger(CommandBuilderTest.class);
	private final CommandBuilder util = new CommandBuilder();
	
	private static final String estimatedBuildCommandAddOrder = "estimatedBuildCommandAddOrder.txt";
	private static final String estimatedBuildCommandAddOrderWithParams = "estimatedBuildCommandAddOrderWithParams.txt";
	private static final String estimatedBuildCommandAddOrderWithParams2 = "estimatedBuildCommandAddOrderWithParams2.txt";
	private static final String estimatedBuildCommandStartJobImmidiately = "estimatedBuildCommandStartJobImmidiately.txt";
	
	
	@Test
	public void addOrderTest() throws IOException {
		String command = util.addOrder("myJobchain").getCommand();
		File file = FileUtils.alwaysCreateEmptyResourceFile(CommandBuilderTest.class, "testBuildCommandAddOrder.txt");
		Files.append(command, file, Charset.defaultCharset());
		File estimated = FileUtils.getResourceFile(CommandBuilderTest.class, estimatedBuildCommandAddOrder);
		compareFiles(file,estimated);
	}

	@Test
	public void addOrderWithParamsTest() throws IOException {
		String command = util.addOrder("myJobchain").addParam("myParam1", "value1").getCommand();
		File file = FileUtils.alwaysCreateEmptyResourceFile(CommandBuilderTest.class, "testBuildCommandAddOrderWithParam.txt");
		Files.append(command, file, Charset.defaultCharset());
		File estimated = FileUtils.getResourceFile(CommandBuilderTest.class, estimatedBuildCommandAddOrderWithParams);
		compareFiles(file,estimated);
	}

	@Test
	public void addOrderWithParams2Test() throws IOException {
		util.addParam("myParam1", "value1");
		util.addParam("myParam2", "value2");
		String command = util.addOrder("myJobchain").getCommand();
		File file = FileUtils.alwaysCreateEmptyResourceFile(CommandBuilderTest.class, "testBuildCommandAddOrderWithParam2.txt");
		Files.append(command, file, Charset.defaultCharset());
		File estimated = FileUtils.getResourceFile(CommandBuilderTest.class, estimatedBuildCommandAddOrderWithParams2);
		compareFiles(file,estimated);
	}

	@Test
	public void showCalendarTest() throws IOException {
		int duration = 60;
		String command = util.showCalendar(duration, What.orders).getCommand();
    	DateTime begin = util.getLastBegin();
    	DateTime end = util.getLastEnd();
    	DateTimeFormatter fmt = ISODateTimeFormat.dateHourMinuteSecond();
    	String estimated = "<show_calendar before='" + fmt.print(end) + "' from='" + fmt.print(begin) + "' limit='10' what='orders'></show_calendar>";
		assertTrue("estimated value is '" + estimated + "'",estimated.equals(command));
	}

	@Test
	public void startJobImmidiatelyTest() throws IOException {
		util.addParam("myParam1", "value1");
		util.addParam("myParam2", "value2");
		String command = util.startJobImmediately("myJob").getCommand();
		File file = FileUtils.alwaysCreateEmptyResourceFile(CommandBuilderTest.class, "testBuildCommandStartJobImmidiately.txt");
		Files.append(command, file, Charset.defaultCharset());
		File estimated = FileUtils.getResourceFile(CommandBuilderTest.class, estimatedBuildCommandStartJobImmidiately);
		compareFiles(file,estimated);
	}
	
	private void compareFiles(File file, File estimated) throws IOException {
        if (!file.exists()) throw new FileNotFoundException(file.getAbsolutePath());
        if (!estimated.exists()) throw new FileNotFoundException(estimated.getAbsolutePath());
		String estimatedContent = Files.toString(estimated, Charset.defaultCharset());
		logger.debug("estimated content: " + estimatedContent);
		assertTrue("content of file '" + file.getAbsolutePath() + "' is not estimated",Files.equal(file, estimated));
	}
	
}
