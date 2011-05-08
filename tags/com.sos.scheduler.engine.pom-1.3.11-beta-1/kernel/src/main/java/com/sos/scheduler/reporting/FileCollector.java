package com.sos.scheduler.reporting;

import java.io.File;
import java.util.ArrayList;

public class FileCollector {
    String directory = null;
    String filenamesStartWith = null;
    String filenamesEndWith = null;
    
    ArrayList<File> files = new ArrayList<File>();
    
    public FileCollector(String directory, String filenamesStartWith, String filenamesEndWith) {
        super();
        this.directory = directory;
        this.filenamesStartWith = filenamesStartWith;
        this.filenamesEndWith = filenamesEndWith;
    }

    public ArrayList<File> collect()
    {
        File f = new File(this.directory);
        scan (f,false);
        return files;
    }
    
    private void scan(File f, boolean delete)
    {
//        System.out.println(f.getAbsolutePath());
        if(f.isDirectory())
        {
            if (f.getName().contains(".skip")) {
                return;
            }
            File[] files = f.listFiles();
            for (int i = 0; i < files.length; i++) {
                scan (files[i], delete);
            }
        }
        if (f.getName().startsWith(this.filenamesStartWith)
                && f.getName().endsWith(this.filenamesEndWith)) 
        {
            if (delete) {
                f.delete();
                System.out.println("file delete:" + f.getAbsolutePath());                
            } else {
                System.out.println("file add:" + f.getAbsolutePath());
                this.files.add(f);
            }
        }
        
    }

    public void delete() {
        File f = new File(this.directory);
        scan (f,true);
        
    }
    

}
