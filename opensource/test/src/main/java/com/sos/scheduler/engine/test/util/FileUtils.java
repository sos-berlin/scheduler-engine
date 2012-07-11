package com.sos.scheduler.engine.test.util;

import com.google.common.io.Resources;
import com.sos.scheduler.engine.kernel.scheduler.SchedulerException;
import com.sos.scheduler.engine.test.TestSchedulerController;

import java.io.File;
import java.net.URISyntaxException;
import java.net.URL;

public class FileUtils {
	
    private final TestSchedulerController controller;
    private final String tempDir;

    private static FileUtils instance = null;
    
    private FileUtils() {
        controller = TestSchedulerController.builder(FileUtils.class).build();
        tempDir = controller.environment().directory().getAbsolutePath();
    }

    public static FileUtils getInstance() {
        if (instance == null)
            instance = new FileUtils();
        return instance;
    }

    public File getTempFile(Class<?> classInstance, String fileWithoutPath) {
        String path = tempDir + "/" + classInstance.getName();
        File dir = new File(path);
        boolean result = dir.mkdirs();
        if (!result)
            throw new SchedulerException("error creating directory " + path);
        return new File( path + "/" + fileWithoutPath );
    }

    public File getResourceFile(String resourceName) {
        URL url = Resources.getResource(resourceName);
        File result = null;
        try {
            result = new File( url.toURI() );
        } catch (URISyntaxException e) {
            throw new SchedulerException("invalid URI '" + url + "' :" + e,e);
        }
        return result;
    }


}
