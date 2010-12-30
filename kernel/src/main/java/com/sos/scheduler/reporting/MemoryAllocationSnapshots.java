package com.sos.scheduler.reporting;

import java.util.ArrayList;

public class MemoryAllocationSnapshots {

    private ArrayList<MemoryAllocationSnapshot> snapshots = new  ArrayList<MemoryAllocationSnapshot>();
    
    public MemoryAllocationSnapshot addSnaphot(String created)
    {
        MemoryAllocationSnapshot snapshot = new MemoryAllocationSnapshot(created, this);
        snapshots.add(snapshot);
        return snapshot;
    }
    
    public void snapshotsPerSchedulerObject()
    {
        
    }
    
    
}
