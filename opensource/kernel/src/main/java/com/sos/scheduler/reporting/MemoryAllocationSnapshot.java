package com.sos.scheduler.reporting;

import java.util.HashMap;

public class MemoryAllocationSnapshot {
    String created;
    private MemoryAllocationSnapshots snapshots;
    
    HashMap<String, Allocation> allocations = new HashMap<String, Allocation>();
    
    public MemoryAllocationSnapshot(String created, MemoryAllocationSnapshots snapshots) {
        super();
        this.created = created;
        this.snapshots = snapshots;
    }

    public void addAllocation(Allocation allocation) {
        this.allocations.put(allocation.name, allocation);
    }

}
