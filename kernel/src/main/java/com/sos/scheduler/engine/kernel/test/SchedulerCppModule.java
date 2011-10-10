package com.sos.scheduler.engine.kernel.test;

import static com.google.common.base.Throwables.propagate;
import static com.sos.scheduler.engine.kernel.test.OperatingSystem.isWindows;

import java.io.File;

import org.apache.log4j.Logger;

final class SchedulerCppModule {
    private static final Logger logger = Logger.getLogger(SchedulerCppModule.class);
    private static final String kernelCppDirName = "kernel-cpp";
    private static final String bin = isWindows? "bind" : "bin"; // Die scheduler.dll wird nur für die Debug-Variante erzeugt

    private final String moduleBase = moduleDirectory() + "/" + "scheduler";
    final File exeFile = new File(OperatingSystem.singleton.makeExecutableFilename(moduleBase)).getAbsoluteFile();
    final File moduleFile = new File(OperatingSystem.singleton.makeModuleFilename(moduleBase)).getAbsoluteFile();

    void load() {
        try {
            logger.debug("load('"+moduleFile+"'), java.library.path="+System.getProperty("java.library.path"));
   		    System.load(moduleFile.getPath());
        } catch (Throwable t) {
            logger.error("load('"+moduleFile+"'): "+t+" - java.library.path="+System.getProperty("java.library.path"));
            throw propagate(t);
        }
    }

    File binDirectory() {
        return moduleFile.getAbsoluteFile().getParentFile();
    }

    /**
     * \brief Determine the library for the scheduler binary
     * \detail
     * For running the JobScheduler in a Java Environment it is neccessary to know where the binary of the
     * Scheduler kernel is stored. In a development landscape it is placed normally in the artifakt 'kernel-cpp'
     * in a subfolder called 'bind'.
     * Use the Scheduler in a single project, e.g. a test case, the scheduler binary has to place in a subfolder
     * called 'lib'.
     *
     * @return String - the name of the library in which the scheduler binary has to put in
     */
    private static String moduleDirectory() {
        File result = new File("./target/lib");
        if (result.exists()) {
        	logger.debug("expecting scheduler binary in '" + result + "'.");
        	return result + "";
        } else {
            logger.debug("Subdirectory 'lib' not found.");
            result = kernelCppDir();
            if (result.exists()) {
                String resultString = result + "/" + bin;
                logger.debug("expecting scheduler binary in '" + resultString + "'.");
                return resultString;
            } else {
                logger.debug("Subdirectory '" + result + "' not found.");
                String msg = "No location for the scheduler binary found - no parent has subdirectory '" + kernelCppDirName + "' and subolder './target/lib' does not exist.";
                logger.error(msg);
                throw new RuntimeException(msg);
            }
        }
    }

    private static File kernelCppDir() {
    	File dir = new File(".");
        while (dir.exists()) {
            File result = new File(dir, kernelCppDirName);
            if (result.exists()) return result;
            dir = new File(dir, "..");
        }
        logger.debug("No parent directory has a subdirectory '" + kernelCppDirName + "'");
        return dir;
//        throw new RuntimeException("No parent directory has a subdirectory '" + kernelCppDirName + "'");
    }
}
