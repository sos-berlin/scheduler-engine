package com.sos.scheduler.engine.data.folder;

import com.sos.scheduler.engine.data.order.OrderKey;

public enum FileBasedType {
    folder("folder", "Folder"),
    job("job", "Job"),
    jobChain("job_chain", "JobChain"),
    lock("lock", "Lock"),
    order("order", "Order"),
    processClass("process_class", "ProcessClass"),
    schedule("schedule", "Schedule");
    //scheduler_script("scheduler_script");

    private final String cppName;
    private final String printName;

    FileBasedType(String cppName, String printName) {
        this.cppName = cppName;
        this.printName = printName;
    }

    public String cppName() {
        return cppName;
    }

    public TypedPath typedPath(String absolutePath) {
        switch (this) {
            case job:
                return new JobPath(absolutePath);
            case jobChain:
                return new JobChainPath(absolutePath);
            //case schedule => new TypedPath(t, absolutePath)
            //case folder =>
            //case lock =>
            case order:
                return OrderKey.apply(absolutePath);
            //case processClass =>
        }
        throw new IllegalArgumentException();
    }

//    public File file(File baseDirectory, Path path) {
//        return new File(baseDirectory, filename(path.string()));
//    }
//
//    public String filename(String name) {
//        return this == folder? name +"/" : name +"."+ cppName +".xml";
//    }

    @Override public String toString() {
        return printName;
    }

    public static FileBasedType fromCppName(String name) {
        for (FileBasedType o: values())
            if (o.cppName.equals(name))
                return o;
        throw new RuntimeException("Unknown file based type '"+name+"'");
    }
}
