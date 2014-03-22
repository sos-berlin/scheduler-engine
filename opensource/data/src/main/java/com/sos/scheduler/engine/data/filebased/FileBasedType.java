package com.sos.scheduler.engine.data.filebased;

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

    <_ extends TypedPath> FileBasedType(String cppName, String printName) {
        this.cppName = cppName;
        this.printName = printName;
    }

    public String cppName() {
        return cppName;
    }

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
