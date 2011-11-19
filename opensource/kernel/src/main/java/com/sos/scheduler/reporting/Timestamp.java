package com.sos.scheduler.reporting;

import java.text.DecimalFormat;
import java.text.ParseException;
import java.util.GregorianCalendar;

import javax.swing.text.NumberFormatter;

public class Timestamp {
    String stringRep;
    String day;
    String month;
    String year;
    String hour;
    String min;
    String sec;
    GregorianCalendar cal;
    
    public Timestamp(String stringRep) {
        super();
        this.stringRep = stringRep;
        day = stringRep.substring(8,9+1);
        month = stringRep.substring(5,6+1);
        year = stringRep.substring(0,3+1);
        hour = stringRep.substring(11,12+1);
        min = stringRep.substring(14,15+1);
        sec = stringRep.substring(17,18+1);
        cal = new GregorianCalendar();
        cal.set(GregorianCalendar.DATE, Integer.parseInt(day));
        cal.set(GregorianCalendar.MONTH, Integer.parseInt(month)-1);
        cal.set(GregorianCalendar.YEAR, Integer.parseInt(year));
        cal.set(GregorianCalendar.HOUR_OF_DAY, Integer.parseInt(hour));
        cal.set(GregorianCalendar.MINUTE, Integer.parseInt(min));
        cal.set(GregorianCalendar.SECOND, Integer.parseInt(sec));

    }
    
    public Timestamp(GregorianCalendar cal) {
        super();
        this.cal = cal;
        
    }

    public GregorianCalendar asInternal()
    {
        return cal;
    }
    
    public String forFilenames () throws ParseException
    {
        String timestamp;
        NumberFormatter formatter = new NumberFormatter();
        formatter.setFormat(new DecimalFormat("00"));
        timestamp = Integer.toString(cal.get(GregorianCalendar.YEAR));
        timestamp += "-" + formatter.valueToString(cal.get(GregorianCalendar.MONTH)+1);
        timestamp += "-" + formatter.valueToString(cal.get(GregorianCalendar.DAY_OF_MONTH));
        timestamp += "T" + formatter.valueToString(cal.get(GregorianCalendar.HOUR_OF_DAY));
        timestamp += "-" + formatter.valueToString(cal.get(GregorianCalendar.MINUTE));
        return timestamp;
    }
    
    
    
}
