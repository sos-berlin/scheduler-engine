package com.sos.scheduler.engine.kernel.util;

//import com.google.common.io.InputSupplier;
import static com.google.common.collect.Maps.fromProperties;

import java.io.IOException;
import java.io.InputStream;
import java.net.URL;
import java.util.Properties;

import com.google.common.collect.ImmutableMap;

public class ClassResource /*implements InputSupplier<InputStream>*/ {
    private final Class<?> clas;
    private final String subPath;


    public ClassResource(Class<?> c, String path) {
        clas = c;
        subPath = path;
    }

    public final InputStream getInputStream() {
        try {
            //clas.getClassLoader().getResourceAsStream(getPath());
            return url().openStream();
        } catch (IOException x) {
            throw new RuntimeException("Java ressource '" + url() + "' is missing", x);
        }
    }

//    private final String getPath() {
//        return clas.getPackage().getName().replace(".", "/") + "/" + subPath;
//    }

    public URL url() {
        return clas.getResource(subPath);
    }

    public ImmutableMap<String,String> properties() {
        try {
            Properties properties = new Properties();
            properties.load(getInputStream());
            return fromProperties(properties);
        } catch (IOException e) { throw new RuntimeException(e); }
    }

    @Override public final String toString() {
        return "Java resource " + url();
    }

    public static URL resourceUrl(Class<?> c, String subPath) {
        return new ClassResource(c, subPath).url();
    }
}
