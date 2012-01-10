package com.sos.scheduler.engine.test.util;

import java.io.File;
import java.io.IOException;
import com.google.common.io.Files;

public class JSFileUtils {
	
	private final static File testresultBasedir = Files.createTempDir();
	
	public static File getTestresultBasedir() {
		return testresultBasedir;
	}

	private JSFileUtils() {
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
		String path = testresultBasedir + "/" + ClassInstance.getPackage().getName().replace(".", "/");
		return createFolderIfNecessary(path);
	}
	
	public static File getTestresultFile(Class<?> ClassInstance, String fileWithoutPath) {
		return new File( getTestresultPath(ClassInstance).getAbsolutePath() + "/" + fileWithoutPath );
	}
	
	public static File createEmptyTestresultFile(Class<?> ClassInstance, String fileWithoutPath) {
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
