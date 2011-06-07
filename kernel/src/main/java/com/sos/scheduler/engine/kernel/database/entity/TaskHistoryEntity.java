package com.sos.scheduler.engine.kernel.database.entity;

import java.util.Date;
import javax.persistence.*;


@Entity
@Table(name="SCHEDULER_HISTORY")
public class TaskHistoryEntity {
    @Id
    private int id;
    
    @Column(name="spooler_id")
    private String spoolerId;

    @Column(name="cluster_member_id")
    private String clusterMemberId;

    @Column(name="job_name")
    private String jobName;

//    @Column(name="start_time")
//    private Date startTime;


    public int getId() {
        return id;
    }


    public void setId(int id) {
        this.id = id;
    }


    public String getSpoolerId() {
        return spoolerId;
    }


    public void setSpoolerId(String spoolerId) {
        this.spoolerId = spoolerId;
    }


    public String getClusterMemberId() {
        return clusterMemberId;
    }


    public void setClusterMemberId(String clusterMemberId) {
        this.clusterMemberId = clusterMemberId;
    }


    public String getJobName() {
        return jobName;
    }


    public void setJobName(String jobName) {
        this.jobName = jobName;
    }


    @Override public String toString() {
        return "spoolerId=" + spoolerId +
                " clusterMemberId=" + clusterMemberId +
                " jobName=" + jobName;
    }
}
