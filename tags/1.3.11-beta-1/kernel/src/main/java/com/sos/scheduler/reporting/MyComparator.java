package com.sos.scheduler.reporting;

import java.util.Comparator;
import java.util.GregorianCalendar;

public class MyComparator implements Comparator<String> {


    @Override
    public int compare(String o1, String o2) {
        //System.out.println(o1 + "---" + o2);
        Timestamp ts1 = new Timestamp(o1);
        Timestamp ts2 = new Timestamp(o2);
        
//        GregorianCalendar cal1 = new GregorianCalendar();
//        GregorianCalendar cal2 = new GregorianCalendar();
//        cal1.set(GregorianCalendar.DATE, Integer.parseInt(o1.substring(8,9+1)));
//        cal1.set(GregorianCalendar.MONTH, Integer.parseInt(o1.substring(5,6+1)));
//        cal1.set(GregorianCalendar.YEAR, Integer.parseInt(o1.substring(0,3+1)));
//        cal1.set(GregorianCalendar.HOUR_OF_DAY, Integer.parseInt(o1.substring(11,12+1)));
//        cal1.set(GregorianCalendar.MINUTE, Integer.parseInt(o1.substring(14,15+1)));
//        cal1.set(GregorianCalendar.SECOND, Integer.parseInt(o1.substring(17,18+1)));
//        cal2.set(GregorianCalendar.DATE, Integer.parseInt(o2.substring(8,9+1)));
//        cal2.set(GregorianCalendar.MONTH, Integer.parseInt(o2.substring(5,6+1)));
//        cal2.set(GregorianCalendar.YEAR, Integer.parseInt(o2.substring(0,3+1)));
//        cal2.set(GregorianCalendar.HOUR_OF_DAY, Integer.parseInt(o2.substring(11,12+1)));
//        cal2.set(GregorianCalendar.MINUTE, Integer.parseInt(o2.substring(14,15+1)));
//        cal2.set(GregorianCalendar.SECOND, Integer.parseInt(o2.substring(17,18+1)));
        if (ts1.asInternal().before(ts2.asInternal()))
            return -1;
        else 
            return 1;
    }

}
