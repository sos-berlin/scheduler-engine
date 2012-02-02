package com.sos.scheduler.engine.test.util;

import java.util.HashMap;

import org.apache.log4j.Logger;
import org.joda.time.DateTime;
import org.joda.time.format.DateTimeFormatter;
import org.joda.time.format.ISODateTimeFormat;

public class JSCommandBuilder {
	
	@SuppressWarnings("unused")
	private static final Logger logger = Logger.getLogger(JSCommandBuilder.class);
	
	private DateTime lastBegin = null;
	private DateTime lastEnd = null;
	private String lastId = null;
	private String commandPrefix;
	private String commandSuffix;
	
	private final HashMap<String,String> params;
	
	public JSCommandBuilder() {
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
	
	public JSCommandBuilder addOrder(String jobchainName) {
		return addOrder(jobchainName,jobchainName);
	}
	
	public JSCommandBuilder addOrder(String jobchainName, String id) {
		lastId = id;
		commandPrefix = "<add_order id='" + getId() + "' job_chain='" + jobchainName + "'>";
		commandSuffix = "</add_order>";
		return this;
	}
	
	public JSCommandBuilder modifyOrder(String order) {
		return modifyOrder(order, order);
	}
	
	public JSCommandBuilder modifyOrder(String order, String id) {
		commandPrefix = "<modify_order at='now' job_chain='" + order + "_chain' order='" + id + "'>";
		commandSuffix = "</modify_order>";
		return this;
	}
	
	public JSCommandBuilder showCalendar(int DurationInSeconds, What what) {
    	lastBegin = new DateTime();
    	lastEnd = new DateTime(lastBegin.plusSeconds(DurationInSeconds));
    	DateTimeFormatter fmt = ISODateTimeFormat.dateHourMinuteSecond();
    	commandPrefix = "<show_calendar before='" + fmt.print(lastEnd) + "' from='" + fmt.print(lastBegin) + "' limit='10' what='" + what + "'>";
    	commandSuffix = "</show_calendar>";
    	return this;
	}
	
	public JSCommandBuilder startJobImmediately(String jobName) {
		commandPrefix = "<start_job job='" + jobName + "' at='now'>";
    	commandSuffix = "</start_job>";
    	return this;
	}
	
	public JSCommandBuilder addParam(String paramName, String paramValue) {
		params.put(paramName, paramValue);
		return this;
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
