package com.sos.scheduler.reporting;

import java.util.ArrayList;

public class SystemProcesses {
    ArrayList<SystemProcess> processes = new ArrayList<SystemProcess>();

    public SystemProcess select(String pid) {
        for (SystemProcess process : this.processes) {
            if (process.pid.equals(pid)) {
                return process;
            }
        }
        SystemProcess process = new SystemProcess(pid);
        processes.add(process);
        return process;
    }
    
    
    
}
