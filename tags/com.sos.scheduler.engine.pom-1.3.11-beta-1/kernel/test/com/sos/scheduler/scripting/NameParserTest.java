package com.sos.scheduler.engine.kernel.scripting;


import org.junit.BeforeClass;
import org.junit.Test;

public class NameParserTest {

    private static NameParser np;
    
    @BeforeClass
    public static void setUpBeforeClass() throws Exception {
        np = new NameParser("init()Z","javascript");
    }
    
    @Test
    public void testParser() {
        System.out.println("Functionname (full): " + np.getNativeFunctionName());
        System.out.println("Functionname: " + np.getFunctionName());
        System.out.println("Functioncall: " + np.getFuntionCall());
        System.out.println("TypeId: " + np.getTypeId());
    }

}
