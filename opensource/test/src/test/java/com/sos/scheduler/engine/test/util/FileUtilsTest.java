package com.sos.scheduler.engine.test.util;

import static org.junit.Assert.assertTrue;
import java.io.File;

import org.apache.log4j.Logger;
import org.junit.Test;

public class FileUtilsTest {
	
	private static final Logger logger = Logger.getLogger(FileUtilsTest.class);

	@Test
	public void testGetLocalResourceFile() {
		
		File file = FileUtils.getResourceFile(FileUtilsTest.class, "myFile.txt");
		String givenFile = file.getAbsolutePath().replace("\\", "/");

        String resourcePath = FileUtils.class.getResource("/").getPath();
		File estimated = new File(resourcePath + this.getClass().getPackage().getName().replace(".", "/") + "/myFile.txt");
		String estimatedFile = estimated.getAbsolutePath().replace("\\", "/");
		
		logger.debug("result ....: " + givenFile);
		logger.debug("estimated .: " + estimatedFile);
		assertTrue("estimated value is '" + estimatedFile + "'",estimatedFile.equals(givenFile));
	}

	@Test
	public void testGetTestResultPath() {
		
		File path = FileUtils.getResourceDir(FileUtilsTest.class);
		String givenPath = path.getAbsolutePath().replace("\\", "/");

		File estimated = new File(FileUtils.getResourceBaseDir() + "/" + this.getClass().getPackage().getName().replace(".", "/"));
		String estimatedPath = estimated.getAbsolutePath().replace("\\", "/");
		
		logger.debug("result ....: " + givenPath);
		logger.debug("estimated .: " + estimatedPath);
		assertTrue("estimated value is '" + estimatedPath + "'",estimatedPath.equals(givenPath));
		assertTrue("folder '" + givenPath + "' does not exist",path.isDirectory());
	}

	@Test
	public void testGetTestResultFile() {
		
		File path = FileUtils.getResourceFile(FileUtilsTest.class, "myFile.txt");
		String givenFile = path.getAbsolutePath().replace("\\", "/");

		File estimated = new File(FileUtils.getResourceBaseDir() + "/"+ this.getClass().getPackage().getName().replace(".", "/") + "/myFile.txt");
		String estimatedFile = estimated.getAbsolutePath().replace("\\", "/");
		
		logger.debug("result ....: " + givenFile);
		logger.debug("estimated .: " + estimatedFile);
		assertTrue("estimated value is '" + estimatedFile + "'",estimatedFile.equals(givenFile));
	}

	@Test
	public void testGetEmptyTestResultFile() {
		
		File file = FileUtils.alwaysCreateEmptyResourceFile(FileUtilsTest.class, "myFile.txt");
		String givenFile = file.getAbsolutePath().replace("\\", "/");

		File estimated = new File(FileUtils.getResourceBaseDir() + "/" + this.getClass().getPackage().getName().replace(".", "/") + "/myFile.txt");
		String estimatedFile = estimated.getAbsolutePath().replace("\\", "/");
		
		logger.debug("result ....: " + givenFile);
		logger.debug("estimated .: " + estimatedFile);
		assertTrue("estimated value is '" + estimatedFile + "'",estimatedFile.equals(givenFile));
		assertTrue("file '" + givenFile + "' does not exist",file.isFile());
		assertTrue("file '" + givenFile + "' is not empty",file.length() == 0);
	}

}
