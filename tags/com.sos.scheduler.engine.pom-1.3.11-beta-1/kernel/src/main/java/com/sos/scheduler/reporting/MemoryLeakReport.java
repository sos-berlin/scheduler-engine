package com.sos.scheduler.reporting;

import java.io.File;
import java.io.FileOutputStream;
import java.text.DecimalFormat;
import java.text.NumberFormat;
import java.util.ArrayList;
import java.util.Collection;
import java.util.Collections;
import java.util.GregorianCalendar;
import java.util.List;
import java.util.Locale;

import javax.swing.text.NumberFormatter;

import org.jdom.Document;
import org.jdom.Element;
import org.jdom.input.SAXBuilder;
import org.jdom.xpath.XPath;

/**
 * 
 * @author rb
 * ben�tigt folgende Jars: jaxen, jdom
 * 
 * 
 */

public class MemoryLeakReport {

    //String reportsLocation = "C:/Program Files/scheduler1.3.8/reports_memory_leaks";
    String reportsLocation = "X:/reports_memory_leaks";

    
    /**
     * @param args
     * @throws Exception 
     */
    public static void main(String[] args) throws Exception {

        MemoryLeakReport memoryLeakReport = new MemoryLeakReport();
        memoryLeakReport.createAllocationReport();
        memoryLeakReport.createAllocationReport2();
        
    }

    /**
     * Transformation XML-Antwort des Job-Scheduler s-Kommandos in csv
     * Eingabedateien in reportsLocation/MemoryAllocationReport*.xml
     * Ausgabedatei pro pid in reportsLocation/MemoryAllocationReport_<start-timestamp-end-timestamp>_<pid>.xml
     * 
     * @throws Exception
     */
    public void createAllocationReport() throws Exception 
    {
        // 1. Teil: Informationen aus XML in Objekte �bertragen
        
        SAXBuilder builder = new SAXBuilder();
        
        SystemProcesses processes = new SystemProcesses();
        
        
        ArrayList<File> files = new FileCollector(
                reportsLocation,
                "MemoryAllocationReport",
                "xml").collect();

        Timestamp created = new Timestamp(new GregorianCalendar());
        FileOutputStream os1 = new FileOutputStream(
                new File(reportsLocation + "/MemoryAllocationReportAddressSpace_" 
                        + created.forFilenames() + ".csv"));

        String csvLine = "pid;created;size;count;reserved_virtual;total_virtual;avail_virtual;total_physical";
        csvLine += ";avail_physical;total_pagefile;avail_pagefile;memoryload\n";
        os1.write(csvLine.getBytes());

        
        for (File f : files) {
            System.out.println("file:" + f.getAbsolutePath());
            Document document = builder.build(f);
            System.out.println(document.getRootElement().getName());
        
            XPath xpath = XPath.newInstance("/spooler/answer");
            Element e = (Element)xpath.selectSingleNode(document);
            String timeStamp = e.getAttributeValue("time");
            System.out.println("Timestamp:" + timeStamp);

            xpath = XPath.newInstance("/spooler/answer/state");
            e = (Element)xpath.selectSingleNode(document);
            String pid = e.getAttributeValue("pid");
            System.out.println("pid:" + pid);
            
            SystemProcess process = processes.select(pid);

            xpath = XPath.newInstance("//spooler/answer/state/allocations");
            e = (Element)xpath.selectSingleNode(document);
            String count = e.getAttributeValue("count");
            String size = e.getAttributeValue("size");

            String reserved_virtual = "";
            String total_virtual = "";
            String avail_virtual = "";
            String total_physical = "";
            String avail_physical = "";
            String total_pagefile = "";
            String avail_pagefile = "";
            String memoryload = "";

            reserved_virtual = e.getAttributeValue("reserved_virtual");            
            if(reserved_virtual != null)
            {
                total_virtual = e.getAttributeValue("total_virtual"); 
                avail_virtual = e.getAttributeValue("avail_virtual");
                total_physical = e.getAttributeValue( "total_physical");
                avail_physical = e.getAttributeValue("avail_physical");
                total_pagefile = e.getAttributeValue( "total_pagefile");
                avail_pagefile = e.getAttributeValue( "avail_pagefile");
                memoryload = e.getAttributeValue( "memoryload");
                csvLine = pid;
                csvLine += ";" + timeStamp;
                csvLine += ";" + size;
                csvLine += ";" + count;
                csvLine += ";" + reserved_virtual;
                csvLine += ";" + total_virtual;
                csvLine += ";" + avail_virtual;
                csvLine += ";" + total_physical;
                csvLine += ";" + avail_physical;
                csvLine += ";" + total_pagefile;
                csvLine += ";" + avail_pagefile;
                csvLine += ";" + memoryload;
                csvLine += "\n";
                os1.write(csvLine.getBytes());
            }

            
            xpath = XPath.newInstance("//spooler/answer/state/allocations/allocation");
            List<Element> list = xpath.selectNodes(document);
            for (Element element : list) {
                System.out.println(
                        "count:" + element.getAttributeValue("count")
                        + " name:" + element.getAttributeValue("name")
                        + " size:" + element.getAttributeValue("size")
                );
                
                
                process.schedulerObjects.addAllocationSnapshot(element.getAttributeValue("name"), 
                        Integer.parseInt(element.getAttributeValue("size")), 
                        Integer.parseInt(element.getAttributeValue("count")), timeStamp);
                

            }
//                break;
        }
        os1.close();
        
        // 2. Teil: Objekte nach csv, html, svg transformieren

        // Aufr�umen
        new FileCollector(
                reportsLocation,
                "MemoryAllocationReport_",
                "csv").delete();

        
        
        
        for (SystemProcess process : processes.processes) {
            
            Collection<String> timestamps = process.schedulerObjects.timestamps.keySet();
            ArrayList<String> timestampsArr = new ArrayList<String>(timestamps);
            Collections.sort(timestampsArr, new MyComparator());
            String startTime = new Timestamp(timestampsArr.get(0)).forFilenames();
            String endTime = new Timestamp(timestampsArr.get(timestampsArr.size()-1)).forFilenames();
            
            FileOutputStream os = new FileOutputStream(
                    new File(reportsLocation + "/MemoryAllocationReport_" + startTime + "-" + endTime + "_" + process.pid + ".csv"));
            
            csvLine = "name;size";
            for (String timestamp : timestampsArr) {
                csvLine += ";" + timestamp;
            }
            csvLine += "\n";
            System.out.println(csvLine);
            os.write(csvLine.getBytes());
            
            Collection<SchedulerObject> objects1 = process.schedulerObjects.objects.values();
            for (SchedulerObject schedulerObject : objects1) {
                csvLine = schedulerObject.name;
                csvLine += ";" + schedulerObject.size;
                for (String timestamp : timestampsArr) {
                    int count = 0;
                    AllocationSnapshot allocationSnapshot = schedulerObject.allocationSnapshots.get(timestamp);
                    if (allocationSnapshot != null) {
                        count = allocationSnapshot.count;
                    }
                    csvLine += ";" + count * schedulerObject.size;
                }
                csvLine += "\n";
                System.out.println(csvLine);
                os.write(csvLine.getBytes());
            }
            csvLine = "Total;'-";
            for (String timestamp : timestampsArr) {
                csvLine += ";" + process.schedulerObjects.timestamps.get(timestamp);
            }
            os.write(csvLine.getBytes());
            
            os.close();
        }
        
        
    }
    

    private String attributeOrNull(Element e, String attributeName)
    {
        String value = e.getAttributeValue(attributeName);
        if(value == null)
            return "0";
        else
            return value;
    }
    
    public void createAllocationReport2() throws Exception
    {
        ArrayList<File> files = new FileCollector(
//                "C:/PerfLog/System/Performance/Sammlungssatz-performance/Sammlungssatz-performance",
                reportsLocation + "/Sammlungssatz-performance",
                "report",
                "xml").collect();
        
        SAXBuilder builder = new SAXBuilder();

        FileOutputStream os = new FileOutputStream(new File( reportsLocation + "/MemoryAllocationReport2" + ".csv"));

        String csvLine = "Shot At;Pid;Private Bytes";
        csvLine += "\n";
        os.write(csvLine.getBytes());
        
        for (File f : files) {
            Document document = builder.build(f);
            System.out.println(document.getRootElement().getName() + "file:" + f.getAbsolutePath());
        
            boolean schedulerProcessFound = false;
            XPath xpath = XPath.newInstance("//Table[@name='TableWorkingSet']/Item/Data[@name='process' and . = 'scheduler']/following-sibling::node()[@name='pid']");
            Element e = (Element)xpath.selectSingleNode(document);
            if (e != null) {
                schedulerProcessFound = true;            
                String pid = e.getText();
                System.out.println("pid:" + pid);
                xpath = XPath.newInstance("//Table[@name='TableWorkingSet']/Item/Data[@name='process' and . = 'scheduler']/following-sibling::node()[@name='private']");
                e = (Element)xpath.selectSingleNode(document);
                String privateBytes = e.getText();
                System.out.println("private bytes:" + privateBytes);    
                
                xpath = XPath.newInstance("//Table[@name='collection']/Item/Data[@name='start']");
                e = (Element)xpath.selectSingleNode(document);
                String year = e.getAttributeValue("year");
                String month = e.getAttributeValue("month");
                String day = e.getAttributeValue("day");
                String hour = e.getAttributeValue("hour");
                String minute = e.getAttributeValue("minute");
                String second = e.getAttributeValue("second");
                String timestamp = year + "-" + month + "-" + day + " " + hour + ":" + minute + ":" + second;
                
                System.out.println("created:" + timestamp );    
                csvLine = "'"+ timestamp + ";" + pid + ";" + privateBytes + "\n";
                os.write(csvLine.getBytes());
//            break;
            }
        }
        os.close();
    }

}
