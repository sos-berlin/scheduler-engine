package com.sos.scheduler.engine.kernel;

import com.sos.scheduler.engine.kernel.util.ClassResource;
import java.io.IOException;
import java.io.InputStream;
import java.util.Properties;


public final class MavenProperties {
    private static final String resourceSimpleName = "maven.properties";
    private final Properties properties;


    public MavenProperties(ClassResource r) {
        try {
            InputStream in = r.getInputStream();
            try {
                properties = new Properties();
                properties.load(in);
            }
            finally { in.close(); }
        }
        catch (IOException x) { throw new RuntimeException(x); }
    }


    public MavenProperties(Class<?> clas) {
        this(new ClassResource(clas, resourceSimpleName));
    }


    public String getGroupId() {
        return get("project.groupId");
    }
    

    public String getArtifactId() {
        return get("project.artifactId");
    }


    public String getVersion() {
        return get("project.version");
    }


    public String get(String name) {
        String result = properties.getProperty(name);
        if (result == null) throw new RuntimeException("Unknown property '" + name + "'");
        return result;
    }


    @Override public String toString() {
        return getGroupId() + "/" + getArtifactId() + " (" + getVersion() + ")";
    }
}
