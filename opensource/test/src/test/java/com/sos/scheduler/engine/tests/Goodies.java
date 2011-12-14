package com.sos.scheduler.engine.tests;

import java.io.File;
import java.io.FileInputStream;

public class Goodies {
	
	private static Goodies instance = null;
	private static final File testresultBaseDir = new File("target/test-results");
	
	protected Goodies() {}
	
	public static Goodies getInstance() {
		if (instance == null) 
			instance = new Goodies();
		return instance;
	}
	
	public void lockFile(String filename, int durationInSeconds) throws Exception {
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
	
	public File getLocalPath(Class<?> ClassInstance) {
		String path = "src/test/java/" + ClassInstance.getPackage().getName().replace(".", "/");
		return createFolderIfNecessary(path);
	}
	
	public File getLocalFile(Class<?> ClassInstance, String fileWithoutPath) {
		return new File( getLocalPath(ClassInstance).getAbsolutePath() + "/" + fileWithoutPath );
	}
	
	public File getTestresultPath(Class<?> ClassInstance) {
		String path = testresultBaseDir + "/" + ClassInstance.getPackage().getName().replace(".", "/");
		return createFolderIfNecessary(path);
	}
	
	public File getTestresultFile(Class<?> ClassInstance, String fileWithoutPath) {
		return new File( getTestresultPath(ClassInstance).getAbsolutePath() + "/" + fileWithoutPath );
	}
	
	private File createFolderIfNecessary(String folderName) {
		File f = new File(folderName);
		boolean result = true;
		if (!f.exists()) result = f.mkdirs();
		if (!result) throw new RuntimeException("error creating folder " + folderName);
		return f;
	}

}
