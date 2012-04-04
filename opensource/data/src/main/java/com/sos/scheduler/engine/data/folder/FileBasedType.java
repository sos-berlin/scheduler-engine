package com.sos.scheduler.engine.data.folder;

public enum FileBasedType {
    folder("folder"),
    job("job"),
    jobChain("job_chain"),
    lock("lock"),
    order("order"),
    processClass("process_class"),
    scheduler("schedule"),
    scheduler_script("scheduler_script");

    private final String cppName;

    FileBasedType(String cppName) {
        this.cppName = cppName;
    }

    public String cppName() {
        return cppName;
    }

    public static FileBasedType fromCppName(String name) {
        for (FileBasedType o: values())
            if (o.cppName.equals(name))
                return o;
        throw new RuntimeException("Unknown file based type '"+name+"'");
    }
}
