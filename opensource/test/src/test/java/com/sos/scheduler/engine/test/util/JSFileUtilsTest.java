package com.sos.scheduler.engine.test.util;

import static org.junit.Assert.*;

import java.io.File;
import java.io.FileNotFoundException;
import java.io.IOException;
import java.nio.charset.Charset;

import org.apache.log4j.Logger;
import org.joda.time.DateTime;
import org.joda.time.format.DateTimeFormatter;
import org.joda.time.format.ISODateTimeFormat;
import org.junit.Ignore;
import org.junit.Test;

import com.google.common.io.Files;

public class JSFileUtilsTest {
	
	private static final Logger logger = Logger.getLogger(JSFileUtilsTest.class);
	
	private final static String lockfile = "fileToLock.txt";
	
	@Test
	public void testGetLocalPath() {
		
		File path = JSFileUtils.getLocalPath(JSFileUtilsTest.class);
		String givenPath = path.getAbsolutePath().replace("\\", "/");

		File estimated = new File("src/test/java/" + this.getClass().getPackage().getName().replace(".", "/"));
		String estimatedPath = estimated.getAbsolutePath().replace("\\", "/");
		
		logger.debug("result ....: " + givenPath);
		logger.debug("estimated .: " + estimatedPath);
		assertTrue("estimated value is '" + estimatedPath + "'",estimatedPath.equals(givenPath));
	}
	
	@Test
	public void testGetLocalResourcePath() {
		
		File path = JSFileUtils.getLocalResourcePath(JSFileUtilsTest.class);
		String givenPath = path.getAbsolutePath().replace("\\", "/");

		File estimated = new File("src/test/resources/" + this.getClass().getPackage().getName().replace(".", "/"));
		String estimatedPath = estimated.getAbsolutePath().replace("\\", "/");
		
		logger.debug("result ....: " + givenPath);
		logger.debug("estimated .: " + estimatedPath);
		assertTrue("estimated value is '" + estimatedPath + "'",estimatedPath.equals(givenPath));
	}

	@Test
	public void testGetLocalFile() {
		
		File file = JSFileUtils.getLocalFile(JSFileUtilsTest.class,"myFile.txt");
		String givenFile = file.getAbsolutePath().replace("\\", "/");

		File estimated = new File("src/test/java/" + this.getClass().getPackage().getName().replace(".", "/") + "/myFile.txt");
		String estimatedFile = estimated.getAbsolutePath().replace("\\", "/");
		
		logger.debug("result ....: " + givenFile);
		logger.debug("estimated .: " + estimatedFile);
		assertTrue("estimated value is '" + estimatedFile + "'",estimatedFile.equals(givenFile));
	}

	@Test
	public void testGetLocalResourceFile() {
		
		File file = JSFileUtils.getLocalResourceFile(JSFileUtilsTest.class,"myFile.txt");
		String givenFile = file.getAbsolutePath().replace("\\", "/");

		File estimated = new File("src/test/resources/" + this.getClass().getPackage().getName().replace(".", "/") + "/myFile.txt");
		String estimatedFile = estimated.getAbsolutePath().replace("\\", "/");
		
		logger.debug("result ....: " + givenFile);
		logger.debug("estimated .: " + estimatedFile);
		assertTrue("estimated value is '" + estimatedFile + "'",estimatedFile.equals(givenFile));
	}

	@Test
	public void testGetTestresultPath() {
		
		File path = JSFileUtils.getTestresultPath(JSFileUtilsTest.class);
		String givenPath = path.getAbsolutePath().replace("\\", "/");

		File estimated = new File("target/test-results/" + this.getClass().getPackage().getName().replace(".", "/"));
		String estimatedPath = estimated.getAbsolutePath().replace("\\", "/");
		
		logger.debug("result ....: " + givenPath);
		logger.debug("estimated .: " + estimatedPath);
		assertTrue("estimated value is '" + estimatedPath + "'",estimatedPath.equals(givenPath));
		assertTrue("folder '" + givenPath + "' does not exist",path.isDirectory());
	}

	@Test
	public void testGetTestresultFile() {
		
		File path = JSFileUtils.getTestresultFile(JSFileUtilsTest.class,"myFile.txt");
		String givenFile = path.getAbsolutePath().replace("\\", "/");

		File estimated = new File("target/test-results/" + this.getClass().getPackage().getName().replace(".", "/") + "/myFile.txt");
		String estimatedFile = estimated.getAbsolutePath().replace("\\", "/");
		
		logger.debug("result ....: " + givenFile);
		logger.debug("estimated .: " + estimatedFile);
		assertTrue("estimated value is '" + estimatedFile + "'",estimatedFile.equals(givenFile));
	}

	@Test
	public void testGetEmptyTestresultFile() {
		
		File file = JSFileUtils.getEmptyTestresultFile(JSFileUtilsTest.class,"myFile.txt");
		String givenFile = file.getAbsolutePath().replace("\\", "/");

		File estimated = new File("target/test-results/" + this.getClass().getPackage().getName().replace(".", "/") + "/myFile.txt");
		String estimatedFile = estimated.getAbsolutePath().replace("\\", "/");
		
		logger.debug("result ....: " + givenFile);
		logger.debug("estimated .: " + estimatedFile);
		assertTrue("estimated value is '" + estimatedFile + "'",estimatedFile.equals(givenFile));
		assertTrue("file '" + givenFile + "' does not exist",file.isFile());
		assertTrue("file '" + givenFile + "' is not empty",file.length() == 0);
	}
	
	/*
	 * TODO Das Sperren des files muß in einem separetem Thread erfolgen, unklar ist, wie der
	 * Hauptprozess mitbekommt, wenn im Thread eine Exception ausgelöst wird (also z.B. die Datei
	 * bereits gesperrt ist). 
	 */
	@Ignore
	public void testLockFile() throws Exception {
		File lock = JSFileUtils.getLocalResourceFile(JSCommandUtilsTest.class, lockfile);
		JSFileUtils.lockFile(lock, 5);
		JSFileUtils.lockFile(lock, 5);
	}
}
