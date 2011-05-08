package com.sos.scheduler.reporting;

import java.util.HashMap;

public class SchedulerObject {
    String name;
    int size;
    HashMap<String, AllocationSnapshot> allocationSnapshots = new HashMap<String, AllocationSnapshot>();
}
