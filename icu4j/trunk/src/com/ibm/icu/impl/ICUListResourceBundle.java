/**
 * A hacked list resource bundle that does redirection 
 * because otherwise some of our resource class files
 * are too big for the java runtime to handle.
 */

package com.ibm.icu.impl;

import java.io.DataInputStream;
import java.io.InputStream;
import java.io.InputStreamReader;
import java.io.IOException;
import java.util.Enumeration;
import java.util.HashMap;
import java.util.Iterator;
import java.util.ListResourceBundle;
import java.util.Map;
import java.util.ResourceBundle;
import java.util.Set;

public class ICUListResourceBundle extends ListResourceBundle {

    protected ICUListResourceBundle() {
    }

	//    protected static final Integer COMPRESSED_BINARY 		= new Integer(1);
    protected static final Integer RESOURCE_BINARY   		= new Integer(2);
	//    protected static final Integer COMPRESSED_STRING	 	= new Integer(3);
	//    protected static final Integer COMPRESSED_BINARY_STRING = new Integer(4);
	protected static final Integer RESOURCE_UNICODE = new Integer(5);

	/**
	 * Subclassers must statically initialize this
	 */
    protected Object[][] contents;

	/**
	 * This is our cache
	 */
	private Object[][] realContents;

    /**
     * See base class description
     */
    protected Object[][] getContents(){
		// we replace any redirected values with real values in a cloned array

		if (realContents == null) {
			realContents = contents;
			for (int i = 0; i < contents.length; ++i) {
				Object newValue = getRedirectedValue(contents[i][1]);
				if (newValue != null) {
					if (realContents == contents) {
						realContents = (Object[][])contents.clone();
					}
					realContents[i] = new Object[] { contents[i][0], newValue };
				}
			}
		}

		return realContents;
    }

	/*
	public void showParents() {
		System.out.println(getClass().getName());
		if (parent == null) {
			System.out.println("parent == null");
		} else if (parent instanceof ICUListResourceBundle) {
			((ICUListResourceBundle)parent).showParents();
		} else {
			System.out.println("parent is not ICUListResourceBundle: " + parent.getClass().getName());
		}
	}
	*/

	/**
	 * Return null if value is already in existing contents array, otherwise fetch the
	 * real value and return it.
	 */
	private Object getRedirectedValue(Object value) {

		// what we really want is:
		// if (value instanceof ICUListResourceBundle.Redirect) {
		//    return ((ICUListResourcBundle.Redirect)value).getValue();
		// }
		// return null;
		// if we really want to support multiple encoding types, then we just have
		// different instances of ICUListResourceBundle.Redirect.

		// value is always an array of one object which is an array of two objects,
		// encoding type and resource name.
		// encoding type is always RESOURCE_BINARY or RESOURCE_UNICODE

		if (value instanceof Object[][]) {
			Object[][] aValue = (Object[][])value;
			if (aValue.length == 1) {
				Object[] bValue = (Object[])aValue[0];
				String resName = (String)bValue[1];
				try {
					if (bValue[0] == RESOURCE_BINARY) {
						// this code would be in RedirectByteArray.getValue, for example, and the resource
						// value would be an instance of this class
						InputStream stream = this.getClass().getResourceAsStream(resName);
						byte[] data = new byte[stream.available()];
						stream.read(data);
						return data; // byte array
					}
					else if (bValue[0] == RESOURCE_UNICODE) {
						InputStream stream = this.getClass().getResourceAsStream(resName);
						InputStreamReader reader = new InputStreamReader(stream);
						char[] data = new char[stream.available() / 2];
						reader.read(data, 0, data.length);
						return data; // char array
					}
				} 
				catch (IOException e) {
					System.out.println("could not load: " + resName);
					System.out.println(e);

					return null;
				}
			}
		}
		
		return null;
	}
}

