/*
 *******************************************************************************
 * Copyright (C) 2016, International Business Machines Corporation and         *
 * others. All Rights Reserved.                                                *
 *******************************************************************************
 */
package com.ibm.icu.dev.test.serializable;

import java.io.ByteArrayInputStream;
import java.io.ByteArrayOutputStream;
import java.io.IOException;
import java.io.ObjectInputStream;
import java.io.ObjectOutputStream;
import java.lang.reflect.Modifier;
import java.net.URL;
import java.util.ArrayList;
import java.util.Enumeration;
import java.util.Iterator;
import java.util.List;

import org.junit.BeforeClass;
import org.junit.Test;
import org.junit.rules.TemporaryFolder;

import com.ibm.icu.dev.test.TestFmwk;
import com.ibm.icu.dev.test.serializable.SerializableTest.Handler;
import com.ibm.icu.impl.URLHandler;

/**
 * @author stuartg
 *
 */
public class CoverageTest2 extends TestFmwk {

    private Class serializable;
    private TemporaryFolder folder;
    
    public CoverageTest2() {
        try {    
            serializable = Class.forName("java.io.Serializable");
        } catch (Exception e) {
            // we're in deep trouble...
            //warnln("Woops! Can't get class info for Serializable.");
        }
        
        // initialize temporary folder
    }
    
    @Test
    public void coverageTest() throws ClassNotFoundException, IOException {
        List<String> classList = generateClassList();
        for (String testClass : classList) {
            testOneClass(testClass);
        }
    }
    
    //@Test
    public void oneOffTest() throws ClassNotFoundException, IOException {
        testOneClass("com.ibm.icu.text.DateFormat");
    }
    
    /**
     * @return
     */
    private List<String> generateClassList() {
        List<String> classList = new ArrayList();
        try {
            Enumeration<URL> urlEnum = getClass().getClassLoader().getResources("com/ibm/icu");
            while (urlEnum.hasMoreElements()) {
                URL url = urlEnum.nextElement();
                URLHandler handler  = URLHandler.get(url);
                if (handler == null) {
                    errln("Unsupported URL: " + url);
                    continue;
                }
                SerializationClassVisitor visitor = new SerializationClassVisitor(classList);
                handler.guide(visitor, true, false);
            }
        } catch (IOException e) {
            throw new RuntimeException(e);
        }

        // TODO(sgill): add randomization support on the list based on the params object
        
        return classList;
    }

    private void testOneClass(String className) throws ClassNotFoundException, IOException {
        Class c = Class.forName(className);
        int m = c.getModifiers();

        if (className.equals("com.ibm.icu.text.PluralRules$FixedDecimal")) {
            // Known Issue: "10268", "Serializable interface is not implemented in PluralRules$FixedDecimal"
            return;
        }
        
        if (c.isEnum() || !serializable.isAssignableFrom(c)) {
            //System.out.println("@@@ Skipping: " + className);
            return;
        }
        if (!Modifier.isPublic(m) || Modifier.isInterface(m)) {
            //System.out.println("@@@ Skipping: " + className);
            return;
        }

        Handler classHandler = SerializableTest.getHandler(className);
        if (classHandler == null) {
            if (!Modifier.isAbstract(m)) {
                //errln("Missing test handler. Update the list of tests in SerializableTest.java to include a test case for " + className);
            }
            return;
        }
        Object[] testObjects = classHandler.getTestObjects();
        byte[] serializedBytes = getSerializedBytes(testObjects);
        Object[] serializedObjects = getSerializedObjects(serializedBytes);
        for (int i = 0; i < testObjects.length; i++) {
            if (!classHandler.hasSameBehavior(serializedObjects[i], testObjects[i])) {
                warnln("Input object " + i + " failed behavior test.");
            }            
        }
    }
    
    /**
     * @param serializedBytes
     * @return
     * @throws IOException 
     * @throws ClassNotFoundException 
     */
    private Object[] getSerializedObjects(byte[] serializedBytes) throws ClassNotFoundException, IOException {
        ByteArrayInputStream byteIS = new ByteArrayInputStream(serializedBytes);
        ObjectInputStream objIS = new ObjectInputStream(byteIS);
        Object inputObjects[] = (Object[]) objIS.readObject();

        objIS.close();
        return inputObjects;
    }

    /**
     * @param testObjects
     * @return
     * @throws IOException 
     */
    private byte[] getSerializedBytes(Object[] objectsOut) throws IOException {
        ByteArrayOutputStream byteOut = new ByteArrayOutputStream();
        ObjectOutputStream objOut = new ObjectOutputStream(byteOut);
        objOut.writeObject(objectsOut);

        byte[] serializedBytes = byteOut.toByteArray();
        objOut.close();
        return serializedBytes;
    }

    private static class SerializationClassVisitor implements URLHandler.URLVisitor {
        private List<String> classNames;

        public SerializationClassVisitor(List<String> classNamesList) {
            this.classNames = classNamesList;
        }
        
        public List<String> getList() {
            return this.classNames;
        }
        
        /* (non-Javadoc)
         * @see com.ibm.icu.impl.URLHandler.URLVisitor#visit(java.lang.String)
         */
        @Override
        public void visit(String classPath) {
            int ix = classPath.lastIndexOf(".class");
            if (ix < 0) {
                return;
            }
            String className = "com.ibm.icu" + classPath.substring(0, ix).replace('/', '.');

            // Skip things in com.ibm.icu.dev; they're not relevant.
            if (className.startsWith("com.ibm.icu.dev.")) {
                return;
            }
            this.classNames.add(className);  
        }
        
    }
}
