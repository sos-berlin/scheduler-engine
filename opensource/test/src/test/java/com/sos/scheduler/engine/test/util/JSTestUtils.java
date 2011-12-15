package com.sos.scheduler.engine.test.util;

import java.io.File;
import java.io.FileInputStream;

public class JSTestUtils {
	
	private static JSTestUtils instance = null;
	private static final File testresultBaseDir = new File("target/test-results");
	
	protected JSTestUtils() {}
	
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
	
	private static File createFolderIfNecessary(String folderName) {
		File f = new File(folderName);
		boolean result = true;
		if (!f.exists()) result = f.mkdirs();
		if (!result) throw new RuntimeException("error creating folder " + folderName);
		return f;
	}

}
