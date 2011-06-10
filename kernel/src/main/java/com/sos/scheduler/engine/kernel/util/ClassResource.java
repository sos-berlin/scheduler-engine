package com.sos.scheduler.engine.kernel.util;

//import com.google.common.io.InputSupplier;
import java.io.InputStream;


public class ClassResource /*implements InputSupplier<InputStream>*/ {
    private final Class<?> clas;
    private final String subPath;


    public ClassResource(Class<?> c, String path) {
        clas = c;
        subPath = path;
    }


    public final InputStream getInputStream() {
        InputStream result = clas.getClassLoader().getResourceAsStream(getPath());
        if (result == null)  throw new RuntimeException("Java ressource '" + getPath() + "' is missing" );
        return result;
    }


//    @Override public InputStream getInput() {
//        return getInputStream();
//    }


    public final String getPath() {
        return clas.getPackage().getName().replace(".", "/") + "/" + subPath;
    }


    @Override public final String toString() {
        return "Java resource " + getPath();
    }
}
