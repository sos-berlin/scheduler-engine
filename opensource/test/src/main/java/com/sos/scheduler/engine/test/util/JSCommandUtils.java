package com.sos.scheduler.engine.test.util;

import java.util.HashMap;

import org.apache.log4j.Logger;
import org.joda.time.DateTime;
import org.joda.time.Period;
import org.joda.time.format.DateTimeFormatter;
import org.joda.time.format.ISODateTimeFormat;

public class JSCommandUtils {
	
	private static final Logger logger = Logger.getLogger(JSCommandUtils.class);
	private static JSCommandUtils instance = null;
	
	private String commandPrefix;
	private String commandSuffix;
	private DateTime begin = null;
	private DateTime end = null;
	
	private HashMap<String,String> params;
	
	protected JSCommandUtils() {
		params = new HashMap<String,String>();
	}
	
	public static JSCommandUtils getInstance() {
		if (instance == null) 
			instance = new JSCommandUtils();
		return instance;
	}

	public String getCommand() {
		String command = commandPrefix;
		if (params.size() > 0 ) {
			command += "<params>";
			for (String key : params.keySet()) {
				command += "<param name='" + key + "' value='" + params.get(key) + "' />";
			}
			command += "</params>";
		}
		command += commandSuffix;
		logger.debug("COMMAND: " + command);
		return command;
	}
	
	public JSCommandUtils buildCommandAddOrder(String jobchainName) {
		commandPrefix = "<add_order id='test_" + jobchainName + "' job_chain='" + jobchainName + "'>";
		commandSuffix = "</add_order>";
		return this;
	}
	
	public JSCommandUtils buildCommandModifyOrder(String order) {
		commandPrefix = "<modify_order at='now' job_chain='" + order + "_chain' order='" + order + "'>";
		commandSuffix = "</modify_order>";
		return this;
	}
	
	public JSCommandUtils buildCommandShowCalendar(int DurationInSeconds, What what) {
    	begin = new DateTime();
    	end = new DateTime(begin.plusSeconds(DurationInSeconds));
    	DateTimeFormatter fmt = ISODateTimeFormat.dateHourMinuteSecond();
    	commandPrefix = "<show_calendar before='" + fmt.print(end) + "' from='" + fmt.print(begin) + "' limit='10' what='" + what + "'>";
    	commandSuffix = "</show_calendar>";
    	initParams();
    	return this;
	}
	
	public DateTime getLastBegin() {
		return begin;
	}
	
	public DateTime getLastEnd() {
		return end;
	}
	
	public void initParams() {
		params.clear();
	}
	
	public JSCommandUtils addParam(String paramName, String paramValue) {
		params.put(paramName, paramValue);
		return this;
	}
}
