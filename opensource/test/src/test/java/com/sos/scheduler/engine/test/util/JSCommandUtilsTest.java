package com.sos.scheduler.engine.test.util;

import static org.junit.Assert.*;

import java.io.File;
import java.io.FileNotFoundException;
import java.io.IOException;
import java.nio.charset.Charset;

import org.apache.log4j.Logger;
import org.joda.time.DateTime;
import org.joda.time.Period;
import org.joda.time.format.DateTimeFormatter;
import org.joda.time.format.ISODateTimeFormat;
import org.junit.Ignore;
import org.junit.Test;

import com.google.common.io.Files;

public class JSCommandUtilsTest {
	
	private static final Logger logger = Logger.getLogger(JSCommandUtilsTest.class);
	private static final JSCommandUtils util = JSCommandUtils.getInstance();
	
	private static final String estimatedBuildCommandAddOrder = "estimatedBuildCommandAddOrder.txt";
	private static final String estimatedBuildCommandAddOrderWithParams = "estimatedBuildCommandAddOrderWithParams.txt";
	private static final String estimatedBuildCommandAddOrderWithParams2 = "estimatedBuildCommandAddOrderWithParams2.txt";
	
	
	@Test
	public void testBuildCommandAddOrder() throws IOException {
		String command = util.buildCommandAddOrder("myJobchain").getCommand();
		File file = JSFileUtils.getEmptyTestresultFile(JSCommandUtilsTest.class,"testBuildCommandAddOrder.txt");
		Files.append(command, file, Charset.defaultCharset());
		File estimated = JSFileUtils.getLocalResourceFile(JSCommandUtilsTest.class, estimatedBuildCommandAddOrder);
		compareFiles(file,estimated);
	}

	@Test
	public void testBuildCommandAddOrderWithParams() throws IOException {
		String command = util.buildCommandAddOrder("myJobchain").addParam("myParam1", "value1").getCommand();
		File file = JSFileUtils.getEmptyTestresultFile(JSCommandUtilsTest.class,"testBuildCommandAddOrderWithParam.txt");
		Files.append(command, file, Charset.defaultCharset());
		File estimated = JSFileUtils.getLocalResourceFile(JSCommandUtilsTest.class, estimatedBuildCommandAddOrderWithParams);
		compareFiles(file,estimated);
	}

	@Test
	public void testBuildCommandAddOrderWithParams2() throws IOException {
		util.addParam("myParam1", "value1");
		util.addParam("myParam2", "value2");
		String command = util.buildCommandAddOrder("myJobchain").getCommand();
		File file = JSFileUtils.getEmptyTestresultFile(JSCommandUtilsTest.class,"testBuildCommandAddOrderWithParam2.txt");
		Files.append(command, file, Charset.defaultCharset());
		File estimated = JSFileUtils.getLocalResourceFile(JSCommandUtilsTest.class, estimatedBuildCommandAddOrderWithParams2);
		compareFiles(file,estimated);
	}

	@Test
	public void testBuildCommandShowCalendar() throws IOException {
		int duration = 60;
		String command = util.buildCommandShowCalendar(duration, What.orders).getCommand();
    	DateTime begin = util.getLastBegin();
    	DateTime end = util.getLastEnd();
    	DateTimeFormatter fmt = ISODateTimeFormat.dateHourMinuteSecond();
    	String estimated = "<show_calendar before='" + fmt.print(end) + "' from='" + fmt.print(begin) + "' limit='10' what='orders'></show_calendar>";
		assertTrue("estimated value is '" + estimated + "'",estimated.equals(command));
	}
	
	private void compareFiles(File file, File estimated) throws IOException {
		if (!estimated.exists()) throw new FileNotFoundException(estimated.getAbsolutePath());
		String estimatedContent = Files.toString(estimated, Charset.defaultCharset());
		logger.debug("estimated content: " + estimatedContent);
		assertTrue("content of file '" + file.getAbsolutePath() + "' is not estimated",Files.equal(file, estimated));
	}
}
