package com.sos.scheduler.engine.test.util;

import com.google.common.io.Resources;
import com.sos.scheduler.engine.kernel.scheduler.SchedulerException;
import java.io.File;
import java.net.URISyntaxException;
import java.net.URL;

public class FileUtils {
	
    private FileUtils() {
    }

    public static File getResourceFile(String resourceName) {
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
