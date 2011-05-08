package com.sos.scheduler.engine.kernelcpptest;

import java.util.Properties;
import java.io.File;
import org.junit.*;
import static org.hamcrest.MatcherAssert.assertThat;
import static org.hamcrest.Matchers.*;
import static com.sos.scheduler.engine.kernelcpptest.OperatingSystemHelper.*;


public class OperatingSystemHelperTest {
    @Test public void testMakeModuleFilename() {
        assertThat(OperatingSystemHelper.singleton.makeModuleFilename("xx"), isWindows? equalTo("xx.dll") : equalTo("libxx.so"));
    }

    @Test public void testMakeExecutableFilename() {
        assertThat(OperatingSystemHelper.singleton.makeExecutableFilename("xx"), isWindows? equalTo("xx.exe") : equalTo("xx"));
    }

    @Test public void testPrependJavaLibraryPath() {
        File f = new File("a/b");
        File a = f.getAbsoluteFile();
        checkAppendJavaLibraryPath("old", f, a.toString() + File.pathSeparator + "old");
    }

    @Test public void testPrependJavaLibraryPathNull() {
        File f = new File("a/b");
        File a = f.getAbsoluteFile();
        checkAppendJavaLibraryPath(null, f, a.toString());
    }

    @Test public void testPrependJavaLibraryPathEmpty() {
        File f = new File("a/b");
        File a = f.getAbsoluteFile();
        checkAppendJavaLibraryPath("", f, a.toString());
    }

    @Test public void testPrependJavaLibraryPathSame1() {
        File f = new File("a/b");
        File a = f.getAbsoluteFile();
        String old = a.toString();
        checkAppendJavaLibraryPath(old, f, old);
    }

    @Test public void testPrependJavaLibraryPathSame2() {
        File f = new File("a/b");
        File a = f.getAbsoluteFile();
        String old = a.toString() + File.pathSeparator + "old";
        checkAppendJavaLibraryPath(old, f, old);
    }

    private static void checkAppendJavaLibraryPath(String old, File added, String result) {
        Properties p = new Properties();
        p.putAll(System.getProperties());
        p.remove(javaLibraryPathPropertyName);
        System.setProperties(p);
        if (old != null)  System.setProperty(javaLibraryPathPropertyName, old);
        prependJavaLibraryPath(added);
        assertThat(System.getProperty(javaLibraryPathPropertyName), equalTo(result));
    }
}