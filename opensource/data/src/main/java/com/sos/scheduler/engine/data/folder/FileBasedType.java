package com.sos.scheduler.engine.data.folder;

import java.io.File;

public enum FileBasedType {
    folder("folder"),
    job("job"),
    jobChain("job_chain"),
    lock("lock"),
    order("order"),
    processClass("process_class"),
    schedule("schedule");
    //scheduler_script("scheduler_script");

    private final String cppName;

    FileBasedType(String cppName) {
        this.cppName = cppName;
    }

    public String cppName() {
        return cppName;
    }

    public TypedPath typedPath(String absolutePath) {
        return new TypedPath(this, new AbsolutePath(absolutePath));
    }

    public TypedPath typedPath(AbsolutePath p) {
        return new TypedPath(this, p);
    }

    public File file(File baseDirectory, Path path) {
        return new File(baseDirectory, filename(path.asString()));
    }

    public String filename(String name) {
        return this == folder? name +"/" : name +"."+ cppName +".xml";
    }

    public static FileBasedType fromCppName(String name) {
        for (FileBasedType o: values())
            if (o.cppName.equals(name))
                return o;
        throw new RuntimeException("Unknown file based type '"+name+"'");
    }
}
