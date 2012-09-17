package com.sos.scheduler.engine.test.util;

import org.apache.log4j.Logger;
import org.joda.time.DateTime;
import org.joda.time.format.DateTimeFormatter;
import org.joda.time.format.ISODateTimeFormat;

import java.util.HashMap;

public class CommandBuilder {
	
	@SuppressWarnings("unused")
	private static final Logger logger = Logger.getLogger(CommandBuilder.class);
	
	private DateTime lastBegin = null;
	private DateTime lastEnd = null;
	private String lastId = null;
	private String commandPrefix;
	private String commandSuffix;
	
	private final HashMap<String,String> params;
	
	public CommandBuilder() {
		params = new HashMap<String,String>();
	}

	public String getCommand() {
		StringBuilder result = new StringBuilder().append(commandPrefix);
		if (params.size() > 0 ) {
			result.append("<params>");
			for (String key : params.keySet()) {
				result.append("<param name='" + key + "' value='" + params.get(key) + "' />");
			}
			result.append("</params>");
		}
		result.append(commandSuffix);
		return result.toString();
	}
	
	public CommandBuilder addOrder(String jobchainName) {
		return addOrder(jobchainName,jobchainName);
	}

    public CommandBuilder addOrder(String jobchainName, String id) {
        lastId = id;
        commandPrefix = "<add_order id='" + getId() + "' job_chain='" + jobchainName + "'>";
        commandSuffix = "</add_order>";
        return this;
    }

    public CommandBuilder addOrderWithState(String jobchainName, String state) {
        lastId = jobchainName + "_" + state;
        commandPrefix = "<add_order id='" + getId() + "' job_chain='" + jobchainName + "' state='" + state + "' >";
        commandSuffix = "</add_order>";
        return this;
    }
	
	public CommandBuilder modifyOrder(String order) {
		return modifyOrder(order, order);
	}
	
	public CommandBuilder modifyOrder(String jobChain, String orderId) {
		commandPrefix = "<modify_order at='now' job_chain='" + jobChain + "' order='" + orderId + "'>";
		commandSuffix = "</modify_order>";
		return this;
	}
	
	public CommandBuilder showCalendar(int DurationInSeconds, What what) {
    	lastBegin = new DateTime();
    	lastEnd = new DateTime(lastBegin.plusSeconds(DurationInSeconds));
    	DateTimeFormatter fmt = ISODateTimeFormat.dateHourMinuteSecond();
    	commandPrefix = "<show_calendar before='" + fmt.print(lastEnd) + "' from='" + fmt.print(lastBegin) + "' limit='10' what='" + what + "'>";
    	commandSuffix = "</show_calendar>";
    	return this;
	}
	
	public CommandBuilder startJobImmediately(String jobName) {
		commandPrefix = "<start_job job='" + jobName + "' at='now'>";
    	commandSuffix = "</start_job>";
    	return this;
	}
	
	public CommandBuilder addParam(String paramName, String paramValue) {
		params.put(paramName, paramValue);
		return this;
	}

    public CommandBuilder killTaskImmediately(String jobName, String id) {
        return killTask(jobName,id,true);
    }

    public CommandBuilder killTask(String jobName, String id, boolean immediately) {
        lastId = id;
        commandPrefix = "<kill_task id='" + getId() + "' job='" + jobName + "' immediately='" + getYesOrNo(immediately) + "'>";
        commandSuffix = "</kill_task>";
        return this;
    }

    private String getYesOrNo(boolean flag) {
        return (flag) ? "yes" : "no";
    }

	public DateTime getLastBegin() {
		return lastBegin;
	}
	
	public DateTime getLastEnd() {
		return lastEnd;
	}

	public String getId() {
		return lastId;
	}
}
