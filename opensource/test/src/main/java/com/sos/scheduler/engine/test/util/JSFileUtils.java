package com.sos.scheduler.engine.test.util;

import java.io.File;
import java.io.IOException;
//import org.apache.log4j.Logger;
import com.google.common.io.Files;

public class JSFileUtils {
	
//	private final static Logger logger = Logger.getLogger(JSFileUtils.class);
	private static final File testresultBaseDir = new File("target/test-results");
	
	private JSFileUtils() {
	}
	
	public static void lockFile(String filename, int durationInSeconds) throws Exception {
		lockFile(new File(filename), durationInSeconds);
	}
	
	public static void lockFile(File file, int durationInSeconds) throws Exception {
		LockFile lock = new LockFile(file, durationInSeconds);
		Thread th = new Thread(lock);
		th.start();
	}
	
	public static File getLocalPath(Class<?> ClassInstance) {
		String path = "src/test/java/" + ClassInstance.getPackage().getName().replace(".", "/");
		return new File(path);
	}
	
	public static File getLocalResourcePath(Class<?> ClassInstance) {
		String path = "src/test/resources/" + ClassInstance.getPackage().getName().replace(".", "/");
		return new File(path);
	}
	
	public static File getLocalFile(Class<?> ClassInstance, String fileWithoutPath) {
		return new File( getLocalPath(ClassInstance).getAbsolutePath() + "/" + fileWithoutPath );
	}
	
	public static File getLocalResourceFile(Class<?> ClassInstance, String fileWithoutPath) {
		return new File( getLocalResourcePath(ClassInstance).getAbsolutePath() + "/" + fileWithoutPath );
	}
	
	public static File getTestresultPath(Class<?> ClassInstance) {
		String path = testresultBaseDir + "/" + ClassInstance.getPackage().getName().replace(".", "/");
		return createFolderIfNecessary(path);
	}
	
	public static File getTestresultFile(Class<?> ClassInstance, String fileWithoutPath) {
		return new File( getTestresultPath(ClassInstance).getAbsolutePath() + "/" + fileWithoutPath );
	}
	
	public static File getEmptyTestresultFile(Class<?> ClassInstance, String fileWithoutPath) {
		File f = getTestresultFile(ClassInstance, fileWithoutPath);
		if (f.exists()) 
			f.delete();
		try {
			Files.touch(f);
		} catch (IOException e) {
			// TODO Auto-generated catch block
			e.printStackTrace();
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

}
