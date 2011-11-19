package com.sos.scheduler.reporting;

import java.util.HashMap;

public class SchedulerObjects {
    HashMap<String,SchedulerObject> objects = new HashMap<String, SchedulerObject>();
    HashMap<String, Integer> timestamps = new HashMap<String, Integer>(); 
    
    public void addAllocationSnapshot(String objectName, int size, int count, String at)
    {
        SchedulerObject schedulerObject = null;
        SchedulerObject search = objects.get(objectName);
        if (search == null) {
            schedulerObject = new SchedulerObject();
            schedulerObject.name = objectName;
            schedulerObject.size = size;
            objects.put(objectName, schedulerObject);
        } else
            schedulerObject = search;
        
        AllocationSnapshot snapshot = new AllocationSnapshot();
        snapshot.at = at;
        snapshot.count = count;
        snapshot.object = schedulerObject;
        schedulerObject.allocationSnapshots.put(at, snapshot);
        
        Integer size1 = timestamps.get(at);
        Integer sizeTotal = null;
        if (size1 == null) {
            sizeTotal = count * snapshot.object.size;
        } else    {
            sizeTotal = size1 + count * snapshot.object.size;
        }
        timestamps.put(at, sizeTotal);            
        
    }
        
}
