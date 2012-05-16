package com.sos.scheduler.engine.test.util;

import com.google.common.io.Resources;
import com.sos.JSHelper.Exceptions.JobSchedulerException;

import java.io.File;
import java.net.URISyntaxException;
import java.net.URL;

public class FileUtils {
	
    private static final String testResultDir = "test-results";
    
    private FileUtils() {}

    public static File getTempFile(Class<?> classInstance, String fileWithoutPath) {
        return new File( getTempDir(classInstance) + "/" + fileWithoutPath );
    }

	private static String getTempDirRoot() {
        URL url = Resources.getResource("");
        File result = null;
        try {
            result = new File(url.toURI());
            String base = (result.getParent() == null) ? result.getAbsolutePath() : result.getParent();
            result = new File(base + "/" + testResultDir);
            if (!result.exists()) result.mkdir();
        } catch (URISyntaxException e) {
            throw new JobSchedulerException("invalid URI '" + url + "': " + e,e);
        }
        return result.getAbsolutePath();
    }

    private static String getTempDir(Class<?> classInstance) {
        String root = getTempDirRoot();
        File result = new File(root + "/" + classInstance.getPackage().getName().replace(".","/"));
        if (!result.exists()) result.mkdirs();
        return result.getAbsolutePath();
    }

    public static File getResourceFile(String resourceName) {
        URL url = Resources.getResource(resourceName);
        File result = null;
        try {
            result = new File( url.toURI() );
        } catch (URISyntaxException e) {
            throw new JobSchedulerException("invalid URI '" + url + "' :" + e,e);
        }
        return result;
    }


}
