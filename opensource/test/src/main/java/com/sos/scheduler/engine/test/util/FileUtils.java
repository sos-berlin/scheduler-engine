package com.sos.scheduler.engine.test.util;

import java.io.File;
import java.io.IOException;
import com.google.common.io.Files;
import com.sos.scheduler.engine.kernel.scheduler.SchedulerException;

public class FileUtils {
	
    private final static String resourcePath = FileUtils.class.getResource("/").getPath();

	public static File getResourceBaseDir() {
		return new File(resourcePath);
	}

	public static File getResourceDir(Class<?> ClassInstance) {
		String path = resourcePath + "/" + ClassInstance.getPackage().getName().replace(".", "/");
		return createFolderIfNecessary(path);
	}
	
	public static File getResourceFile(Class<?> ClassInstance, String fileWithoutPath) {
		return new File( getResourceDir(ClassInstance).getAbsolutePath() + "/" + fileWithoutPath );
	}
	
	public static File alwaysCreateEmptyResourceFile(Class<?> ClassInstance, String fileWithoutPath) {
		File f = getResourceFile(ClassInstance, fileWithoutPath);
		if (f.exists()) 
			f.delete();
		try {
			Files.touch(f);
		} catch (IOException e) {
            throw new SchedulerException("file " + f.getAbsolutePath() + "could not created.",e);
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
