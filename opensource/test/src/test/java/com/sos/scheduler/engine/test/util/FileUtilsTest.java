package com.sos.scheduler.engine.test.util;

import org.apache.log4j.Logger;

public class FileUtilsTest {
	
	private static final Logger logger = Logger.getLogger(FileUtilsTest.class);

    /*
	@Test
	public void testGetResourceDir() {
		
		File resourceDir = FileUtils.getResourceDir(FileUtilsTest.class);
		String resourcePath = resourceDir.getAbsolutePath().replace("\\", "/");

		File expectedDir = new File(FileUtils.getResourceBaseDir() + "/" + this.getClass().getPackage().getName().replace(".", "/"));
		String expectedPath = expectedDir.getAbsolutePath().replace("\\", "/");
		
		logger.debug("resource path ....: " + resourcePath);
		logger.debug("expected path ....: " + expectedPath);
		assertTrue("estimated value is '" + expectedPath + "'", expectedPath.equals(resourcePath));
		assertTrue("folder '" + resourcePath + "' does not exist",resourceDir.isDirectory());
	}
	*/

    /*
	@Test
	public void testGetResourceFile() {
		
		File resourceFile = FileUtils.getTempFile(FileUtilsTest.class, "myFile.txt");
		String resourceName = resourceFile.getAbsolutePath().replace("\\", "/");

		File expectedFile = new File(FileUtils.getResourceBaseDir() + "/"+ this.getClass().getPackage().getName().replace(".", "/") + "/myFile.txt");
		String expectedName = expectedFile.getAbsolutePath().replace("\\", "/");
		
		logger.debug("resourcename ....: " + resourceName);
		logger.debug("expected name ...: " + expectedName);
		assertTrue("expected name value is '" + expectedName + "'",expectedName.equals(resourceName));
	}
	*/

}
