package com.sos.scheduler.engine.test.util;

import com.google.common.io.Files;
import com.google.common.io.Resources;
import com.sos.scheduler.engine.kernel.scheduler.SchedulerException;
import com.sos.scheduler.engine.test.TestSchedulerController;

import java.io.File;
import java.io.IOException;
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
        String filename = tempDir.replace("\\","/") + "/" + classInstance.getName() + "/" + fileWithoutPath;
        File resultFile = new File(filename);
        try {
            Files.createParentDirs(resultFile);
        } catch (IOException e) {
            throw new SchedulerException("error creating directory for file " + filename);
        }
        return resultFile;
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
