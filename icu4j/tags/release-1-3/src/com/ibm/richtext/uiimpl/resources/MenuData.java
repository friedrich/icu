/*
 * @(#)$RCSfile: MenuData.java,v $ $Revision: 1.1 $ $Date: 2000/04/20 17:52:46 $
 *
 * (C) Copyright IBM Corp. 1998-1999.  All Rights Reserved.
 *
 * The program is provided "as is" without any warranty express or
 * implied, including the warranty of non-infringement and the implied
 * warranties of merchantibility and fitness for a particular purpose.
 * IBM will not be liable for any damages suffered by you as a result
 * of using the Program. In no event will IBM be liable for any
 * special, indirect or consequential damages or lost profits even if
 * IBM has been advised of the possibility of their occurrence. IBM
 * will not be liable for any third party claims against you.
 */
package com.ibm.richtext.uiimpl.resources;

/**
 * This class is used in resources to represent a Menu.  It is
 * just a name and an optional shortcut key.
 */
public final class MenuData {

    static final String COPYRIGHT =
                "(C) Copyright IBM Corp. 1998-1999 - All Rights Reserved";
    private String fName;
    private boolean fHasShortcut;
    private char fShortcut;
    private int fKeyCode;

    public MenuData(String name) {

        fName = name;
        fHasShortcut = false;
    }

    public MenuData(String name, char ch, int keyCode) {

        fName = name;
        fHasShortcut = true;
        fShortcut = ch;
        fKeyCode = keyCode;
    }

    public String getName() {

        return fName;
    }

    public char getShortcutChar() {

        if (!fHasShortcut) {
            throw new Error("Menu doesn't have shortcut");
        }
        return fShortcut;
    }
    
    public int getShortcutKeyCode() {
        if (!fHasShortcut) {
            throw new Error("Menu doesn't have shortcut");
        }
        return fKeyCode;
    }

    public boolean hasShortcut() {

        return fHasShortcut;
    }
}