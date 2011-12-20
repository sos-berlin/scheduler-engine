package com.sos.scheduler.engine.test.util;

import java.io.File;
import java.io.FileInputStream;
import java.io.IOException;
import java.util.HashMap;

import org.joda.time.DateTime;
import org.joda.time.format.DateTimeFormatter;
import org.joda.time.format.ISODateTimeFormat;

public class JSTestUtils {
	
	private static JSTestUtils instance = null;
	private static final File testresultBaseDir = new File("target/test-results");
	
	private HashMap<String,String> params;
	
	protected JSTestUtils() {
		params = new HashMap<String,String>();
	}
	
	public static JSTestUtils getInstance() {
		if (instance == null) 
			instance = new JSTestUtils();
		return instance;
	}
	
	public static void lockFile(String filename, int durationInSeconds) throws Exception {
		FileInputStream in = new FileInputStream(filename);
		try {
		    java.nio.channels.FileLock lock = in.getChannel().tryLock(0L, Long.MAX_VALUE, true);
		    try {
		    	Thread.sleep(durationInSeconds*1000);
		    } finally {
		        lock.release();
		    }
		} finally {
		    in.close();
		}
	}
	
	public static File getLocalPath(Class<?> ClassInstance) {
		String path = "src/test/java/" + ClassInstance.getPackage().getName().replace(".", "/");
		return createFolderIfNecessary(path);
	}
	
	public static File getLocalFile(Class<?> ClassInstance, String fileWithoutPath) {
		return new File( getLocalPath(ClassInstance).getAbsolutePath() + "/" + fileWithoutPath );
	}
	
	public static File getTestresultPath(Class<?> ClassInstance) {
		String path = testresultBaseDir + "/" + ClassInstance.getPackage().getName().replace(".", "/");
		return createFolderIfNecessary(path);
	}
	
	public static File getTestresultFile(Class<?> ClassInstance, String fileWithoutPath) {
		return new File( getTestresultPath(ClassInstance).getAbsolutePath() + "/" + fileWithoutPath );
	}
	
	public static File getEmptyTestresultFile(Class<?> ClassInstance, String fileWithoutPath) throws IOException {
		File f = getTestresultFile(ClassInstance, fileWithoutPath);
		if (f.exists()) {
			f.delete();
			f.createNewFile();
		}
		return f;
	}
	
	private static File createFolderIfNecessary(String folderName) {
		File f = new File(folderName);
		boolean result = true;
		if (!f.exists()) result = f.mkdirs();
		if (!result) throw new RuntimeException("error creating folder " + folderName);
		return f;
	}
	
	public String buildCommandAddOrder(String jobchainName) {
		String command = "<add_order id='test_" + jobchainName + "' job_chain='" + jobchainName + "'>";
		addParams(command);
		command += "</add_order>";
		return command;
	}
	
	public String buildCommandModifyOrder(String order) {
		String command = "<modify_order at='now' job_chain='" + order + "_chain' order='" + order + "'>";
		addParams(command);
		command += "</modify_order>";
		return command;
	}
	
	public String buildCommandShowCalendar(int DurationInSeconds, What what) {
    	DateTime begin = new DateTime();
    	DateTime end = new DateTime(begin.plusSeconds(DurationInSeconds));
    	DateTimeFormatter fmt = ISODateTimeFormat.dateHourMinuteSecond();
    	String command = "<show_calendar before='" + fmt.print(end) + "' from='" + fmt.print(begin) + "' limit='10' what='" + what + "' />";
		return command;
	}
	
	private void addParams(String command) {
		if (params.size() > 0 ) {
			command += "<params>";
			for (String key : params.keySet()) {
				command += "<param name='" + key + "' value='" + params.get(key) + "' />";
			}
			command += "</params>";
		}
	}
	
	public void initParams() {
		params.clear();
	}
	
	public void addParam(String paramName, String paramValue) {
		params.put(paramName, paramValue);
	}
}
