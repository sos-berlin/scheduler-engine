package com.sos.scheduler.kernel.core.scripting;

import org.junit.BeforeClass;
import org.junit.Test;


public class ModuleTest {

    @BeforeClass
    public static void setUpBeforeClass() throws Exception {
    }
    
    @Test
    public void TestModule_JavaScript() {
        try {
            System.out.println("----------------------------------------------------- TestModule_JavaScript");
            String code = "function addVar(input) {\n" +
                          "    var result = Math.abs(val) + Math.abs(input);\n" +
                          "    return result;\n" +
                          "}";
            Module mi = new Module("java:javascript",code);
            mi.addObject("15", "val");
            Double result = mi.callDouble("addVar(12)Z");
            System.out.println("12 + 15 = " + result);
        } catch(Exception e) {
            e.printStackTrace();
        }
    }
    
    @Test
    public void TestModule_JavaScriptWithError() {
        try {
            System.out.println("----------------------------------------------------- TestModule_JavaScriptWithError");
            String code = "function addVar(input) {\n" +
                          "    return ( val(val) + val(input) );\n" +
                          "}";
            Module mi = new Module("java:javascript",code);
            mi.addObject("15", "val");
            String result = mi.callString("addVar(12)Z");
            System.out.println(result);
        } catch(Exception e) {
            e.printStackTrace();
        }
    }
    
    @Test
    public void TestModule_Judo() {
        try {
            System.out.println("----------------------------------------------------- TestModule_Judo");
            String code = "" +
                          "const #PI = 3.1415926536 ifndef;\n" + 
                          "println '    #PI = ', #PI : 2.7;";
            Module mi = new Module("java:judo",code);
            // mi.addObject("15", "val");
            mi.call("#! /usr/bin/env judo");
            //System.out.println(result);
        } catch(Exception e) {
            e.printStackTrace();
        }
    }
    
}
