/**
*******************************************************************************
* Copyright (C) 1996-2000, International Business Machines Corporation and    *
* others. All Rights Reserved.                                                *
*******************************************************************************
*
* $Source: /xsrl/Nsvn/icu/icu4j/src/com/ibm/icu/text/Attic/UCharacterDB.java,v $ 
* $Date: 2000/12/26 20:00:56 $ 
* $Revision: 1.1 $
*
*******************************************************************************
*/
package com.ibm.icu.text;

/**
* Internal base class for all character databases.
* Database classes store binary data read from uprops.dat and unames for use. 
* It does not have the capability to parse the data into more high-level 
* information. It only returns bytes of information when required. 
* Due to the form most commonly used for retrieval, array of char is used
* to store the binary data
* Responsibility for molding the binary data into more meaning form lies on 
* <a href=UCharacterPpty.html>UCharacterPpty</a> and 
* <a href=UCharacterName.html>UCharacterName</a>.
* Data populated by <a href=UGenReader.html>UGenReader</a>
* @author Syn Wee Quek
* @since oct3100 HALLOWEEN!!
* @see com.ibm.icu.text.UCharacterPpty
* @see com.ibm.icu.text.UCharacterName
*/

class UCharacterDB
{
  // protected variable ===========================================  
  
  /**
  * Unicode data version
  */
  String m_unicodeversion_;
  
  // constructor =============================================
  
  /**
  * Constructor for UCharacterDB
  */
  protected UCharacterDB()
  {
  }
  
  // public method =============================================
  
  /**
   * toString method for printing
   */
  public String toString()
  {
    StringBuffer result = new StringBuffer();
    /*for (int i = 0; i < size; i ++)
    {
      result.append(" ");
      result.append(0x0000FFFF & m_db_[i]);
    }
    
    result.append('\n');
    */
    result.append("\nunicode version number ");
    result.append(m_unicodeversion_);
    
    return result.toString();
  }
  
  // protected method =============================================
  
  /**
  * set version number for this set of unicode characters
  * @param version
  * @return false if version is not a valid number
  */
  protected boolean setUnicodeVersion(byte[] version)
  {
    int size = 0;
    if (version != null)
      size = version.length;
    boolean result = false;
    StringBuffer s = new StringBuffer(size);
    for (int i = 0; i < size; i++)
    {
      s.append((int)version[i]);
      s.append('.');
      if (version[i] < 0 || version[i] > 9)
        return false;
      if (version[i] != 0)
        result = true;
    }
    if (result)
      m_unicodeversion_ = s.substring(0, (size << 1) - 1);
    return true;
  }
}