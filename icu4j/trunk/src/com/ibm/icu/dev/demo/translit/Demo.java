/*
 *******************************************************************************
 * Copyright (C) 1996-2000, International Business Machines Corporation and    *
 * others. All Rights Reserved.                                                *
 *******************************************************************************
 *
 * $Source: /xsrl/Nsvn/icu/icu4j/src/com/ibm/icu/dev/demo/translit/Demo.java,v $ 
 * $Date: 2002/07/15 01:26:18 $ 
 * $Revision: 1.21 $
 *
 *****************************************************************************************
 */
package com.ibm.icu.dev.demo.translit;
import java.applet.*;
import java.awt.*;
import java.awt.event.*;
import java.util.*;
import java.text.CharacterIterator;
import com.ibm.icu.dev.demo.impl.*;
import com.ibm.icu.lang.*;
import com.ibm.icu.text.*;
import com.ibm.icu.impl.*;
import java.io.*;

/**
 * A frame that allows the user to experiment with keyboard
 * transliteration.  This class has a main() method so it can be run
 * as an application.  The frame contains an editable text component
 * and uses keyboard transliteration to process keyboard events.
 *
 * <p>Copyright (c) IBM Corporation 1999.  All rights reserved.
 *
 * @author Alan Liu
 * @version $RCSfile: Demo.java,v $ $Revision: 1.21 $ $Date: 2002/07/15 01:26:18 $
 */
public class Demo extends Frame {

    static final boolean DEBUG = false;
    static final String START_TEXT = "(cut,\u03BA\u03C5\u03C4,\u05D0,\u30AF\u30C8,\u4E80,\u091A\u0941\u0924\u094D)";

    Transliterator translit = null;
    String fontName = "Arial Unicode MS";
    int fontSize = 18;
    
    

    /*
    boolean compound = false;
    Transliterator[] compoundTranslit = new Transliterator[MAX_COMPOUND];
    static final int MAX_COMPOUND = 128;
    int compoundCount = 0;
    */

    TransliteratingTextComponent text = null;

    Menu translitMenu;
    CheckboxMenuItem translitItem;
    CheckboxMenuItem noTranslitItem;

    static final String NO_TRANSLITERATOR = "None";

    private static final String COPYRIGHT =
        "\u00A9 IBM Corporation 1999. All rights reserved.";

    public static void main(String[] args) {
        Frame f = new Demo(600, 200);
        f.addWindowListener(new WindowAdapter() {
            public void windowClosing(WindowEvent e) {
                System.exit(0);
            }
        });
        f.setVisible(true);
    }

	public Demo(int width, int height) {
        super("Transliteration Demo");

        initMenus();

        addWindowListener(new WindowAdapter() {
            public void windowClosing(WindowEvent e) {
                handleClose();
            }
        });
        
        text = new TransliteratingTextComponent();
        Font font = new Font(fontName, Font.PLAIN, fontSize);
        text.setFont(font);
        text.setSize(width, height);
        text.setVisible(true);
        text.setText(START_TEXT);
        add(text);

        setSize(width, height);
        setTransliterator("Latin-Greek", null);
    }

    private void initMenus() {
        MenuBar mbar;
        Menu menu;
        MenuItem mitem;
        CheckboxMenuItem citem;
        
        setMenuBar(mbar = new MenuBar());
        mbar.add(menu = new Menu("File"));
        menu.add(mitem = new MenuItem("Quit"));
        mitem.addActionListener(new ActionListener() {
            public void actionPerformed(ActionEvent e) {
                handleClose();
            }
        });
/*
        final ItemListener setTransliteratorListener = new ItemListener() {
            public void itemStateChanged(ItemEvent e) {
                CheckboxMenuItem item = (CheckboxMenuItem) e.getSource();
                if (e.getStateChange() == ItemEvent.DESELECTED) {
                    // Don't let the current transliterator be deselected.
                    // Just reselect it.
                    item.setState(true);
                } else if (compound) {
                    // Adding an item to a compound transliterator
                    handleAddToCompound(item.getLabel());
                } else if (item != translitItem) {
                    // Deselect previous choice.  Don't need to call
                    // setState(true) on new choice.
                    translitItem.setState(false);
                    translitItem = item;
                    handleSetTransliterator(item.getLabel());
                }
            }
        };
*/
        /*
        translitMenu.add(translitItem = noTranslitItem =
                         new CheckboxMenuItem(NO_TRANSLITERATOR, true));
        noTranslitItem.addItemListener(new ItemListener() {
            public void itemStateChanged(ItemEvent e) {
                // Can't uncheck None -- any action here sets None to true
                setNoTransliterator();
            }
        });

        translitMenu.addSeparator();
        */

/*
        translitMenu.add(citem = new CheckboxMenuItem("Compound"));
        citem.addItemListener(new ItemListener() {
            public void itemStateChanged(ItemEvent e) {
                CheckboxMenuItem item = (CheckboxMenuItem) e.getSource();
                if (e.getStateChange() == ItemEvent.DESELECTED) {
                    // If compound gets deselected, then select NONE
                    setNoTransliterator();
                } else if (!compound) {
                    // Switching from non-compound to compound
                    translitItem.setState(false);
                    translitItem = item;
                    translit = null;
                    compound = true;
                    compoundCount = 0;
                    for (int i=0; i<MAX_COMPOUND; ++i) {
                        compoundTranslit[i] = null;
                    }
                }
            }
        });
      
        translitMenu.addSeparator();
       */

        /*
        for (Enumeration e=getSystemTransliteratorNames().elements();
             e.hasMoreElements(); ) {
            String s = (String) e.nextElement();
            translitMenu.add(citem = new CheckboxMenuItem(s));
            citem.addItemListener(setTransliteratorListener);
        }
        */
        
        Menu fontMenu = new Menu("Font");
        String[] fonts = GraphicsEnvironment.getLocalGraphicsEnvironment().getAvailableFontFamilyNames();
        for (int i = 0; i < fonts.length; ++i) {
            MenuItem mItem = new MenuItem(fonts[i]);
            mItem.addActionListener(new FontActionListener(fonts[i]));
            fontMenu.add(mItem);
        }
        mbar.add(fontMenu);
        
        Menu sizeMenu = new Menu("Size");
        int[] sizes = {9, 10, 12, 14, 18, 24, 36, 48, 72};
        for (int i = 0; i < sizes.length; ++i) {
            MenuItem mItem = new MenuItem("" + sizes[i]);
            mItem.addActionListener(new SizeActionListener(sizes[i]));
            sizeMenu.add(mItem);
        }
        mbar.add(sizeMenu);
        
        translit = null;
        
        mbar.add(translitMenu = new Menu("Transliterator"));
        
        translitMenu.add(convertSelectionItem = new MenuItem("Transliterate", 
            new MenuShortcut(KeyEvent.VK_K)));
        convertSelectionItem.addActionListener(new ActionListener() {
            public void actionPerformed(ActionEvent e) {
                handleBatchTransliterate(translit);
            }
        });
        
        translitMenu.add(swapSelectionItem = new MenuItem("Reverse", 
            new MenuShortcut(KeyEvent.VK_S)));
        swapSelectionItem.addActionListener(new ActionListener() {
            public void actionPerformed(ActionEvent e) {
            	Transliterator inv;
            	try {
                	inv = translit.getInverse();
                } catch (Exception x) {
                	inv = new NullTransliterator();
                }
            	setTransliterator(inv.getID(), null);
            }
        });
        
        translitMenu.add(convertTypingItem = new MenuItem("No Typing Conversion",
            new MenuShortcut(KeyEvent.VK_T)));
        convertTypingItem.addActionListener(new ActionListener() {
            public void actionPerformed(ActionEvent e) {
                if (!transliterateTyping) {
                    text.setTransliterator(translit);
                    convertTypingItem.setLabel("No Typing Conversion");
                } else {
                    text.flush();
                    text.setTransliterator(null);
                    convertTypingItem.setLabel("Convert Typing");
                }
                transliterateTyping = !transliterateTyping;
            }
        });
        
        translitMenu.add(historyMenu = new Menu("Recent"));
        
        helpDialog = new InfoDialog(this, "Simple Demo", "Instructions",
           "CTL A, X, C, V have customary meanings.\n"
         + "Arrow keys, delete and backspace work.\n"
         + "To get a character from its control point, type the hex, then hit CTL Q"
        );
        helpDialog.getArea().setEditable(false);
        
       
        Menu helpMenu;
        mbar.add(helpMenu = new Menu("Extras"));
        helpMenu.add(mitem = new MenuItem("Help"));
        mitem.addActionListener(new ActionListener() {
            public void actionPerformed(ActionEvent e) {
                helpDialog.show();
            }
        });   
        
        hexDialog = new InfoDialog(this, "Hex Entry", "Use U+..., \\u..., \\x{...}, or &#x...;",
           "\\u00E1"
        );
        Button button = new Button("Insert");
        button.addActionListener(new ActionListener() {
            public void actionPerformed(ActionEvent e) {
                String hexValue = hexDialog.getArea().getText();
                text.insertText(fromHex.transliterate(hexValue));
            }
        });
        hexDialog.getBottom().add(button);
        
        helpMenu.add(mitem = new MenuItem("Hex...", 
            new MenuShortcut(KeyEvent.VK_H)));
        mitem.addActionListener(new ActionListener() {
            public void actionPerformed(ActionEvent e) {
                hexDialog.show();
            }
        });
        
        // Compound Transliterator
        
        compoundDialog = new InfoDialog(this, "Compound Transliterator", "",
           "[^\\u0000-\\u00FF] hex"
        );
        button = new Button("Set");
        button.addActionListener(new ActionListener() {
            public void actionPerformed(ActionEvent e) {
                String compound = "";
                try {
                    compound = compoundDialog.getArea().getText();
                    setTransliterator(compound, null);
                } catch (RuntimeException ex) {
                    compoundDialog.getArea().setText(compound + "\n" + ex.getMessage());
                }
            }
        });
        compoundDialog.getBottom().add(button);
        
        translitMenu.add(mitem = new MenuItem("Multiple...", 
            new MenuShortcut(KeyEvent.VK_M)));
        mitem.addActionListener(new ActionListener() {
            public void actionPerformed(ActionEvent e) {
                compoundDialog.show();
            }
        });
        
        // RuleBased Transliterator
        
        rulesDialog = new InfoDialog(this, "Rule-Based Transliterator", "",
           "([A-Z]) > &Hex($1) &Name($1);\r\n" 
            + "&Hex-Any($1) < ('\\' [uU] [a-fA-F0-9]*);\r\n" 
			+ "&Name-Any($1) < ('{' [^\\}]* '}');"
        );
        button = new Button("Set");
        button.addActionListener(new ActionListener() {
            public void actionPerformed(ActionEvent e) {
                String compound = "";
                try {
                    compound = rulesDialog.getArea().getText();
                    String id = ruleId.getText();
                    setTransliterator(compound, id);
                } catch (RuntimeException ex) {
                    rulesDialog.getArea().setText(compound + "\n" + ex.getMessage());
                }
            }
        });
        rulesDialog.getBottom().add(button);
        ruleId = new TextField("test1", 20);
        Label temp = new Label(" Name:");
        rulesDialog.getBottom().add(temp);
        rulesDialog.getBottom().add(ruleId);
        
        
        translitMenu.add(mitem = new MenuItem("From Rules...", 
            new MenuShortcut(KeyEvent.VK_R)));
        mitem.addActionListener(new ActionListener() {
            public void actionPerformed(ActionEvent e) {
                rulesDialog.show();
            }
        });
        
        
        translitMenu.add(mitem = new MenuItem("From File...", 
            new MenuShortcut(KeyEvent.VK_F)));
        mitem.addActionListener(new FileListener(this, RULE_FILE));
        
        translitMenu.add(mitem = new MenuItem("Test File..."));
        mitem.addActionListener(new FileListener(this, TEST_FILE));
        
        // Flesh out the menu with the installed transliterators
        
        translitMenu.addSeparator();
        
        Iterator sources = add(new TreeSet(), Transliterator.getAvailableSources()).iterator();
        while(sources.hasNext()) {
            String source = (String) sources.next();
            Iterator targets = add(new TreeSet(), Transliterator.getAvailableTargets(source)).iterator();
            Menu targetMenu = new Menu(source);
            while(targets.hasNext()) {
                String target = (String) targets.next();
                Set variantSet = add(new TreeSet(), Transliterator.getAvailableVariants(source, target));
                if (variantSet.size() < 2) {
                    mitem = new MenuItem(target);
                    mitem.addActionListener(new TransliterationListener(source + "-" + target));
                    targetMenu.add(mitem);
                } else {
                    Iterator variants = variantSet.iterator();
                    Menu variantMenu = new Menu(target);
                    while(variants.hasNext()) {
                        String variant = (String) variants.next();
                        mitem = new MenuItem(variant == "" ? "<default>" : variant);
                        mitem.addActionListener(new TransliterationListener(source + "-" + target + "/" + variant));
                        variantMenu.add(mitem);
                    }
                    targetMenu.add(variantMenu);
                }
            }
            translitMenu.add(targetMenu);
        }
        
        
    }
    
    static final int RULE_FILE = 0, TEST_FILE = 1;
    //
    static class FileListener implements ActionListener {
        Demo frame;
        int choice;
        
        FileListener(Demo frame, int choice) {
            this.frame = frame;
            this.choice = choice;
        }
        
        public void actionPerformed(ActionEvent e) {
            FileDialog fileDialog = new FileDialog(frame, "Input File");
            fileDialog.show();
            String fileName = fileDialog.getFile();
            String fileDirectory = fileDialog.getDirectory();
            if (fileName != null) {
                try {
                    File f = new File(fileDirectory, fileName);
                    if (choice == RULE_FILE) {
                        StringBuffer buffer = new StringBuffer();
                        FileInputStream fis = new FileInputStream(f);
                        InputStreamReader isr = new InputStreamReader(fis, "UTF8");
                        BufferedReader br = new BufferedReader(isr, 32*1024);
                        while (true) {
                            String line = br.readLine();
                            if (line == null) break;
                            if (line.length() > 0 && line.charAt(0) == '\uFEFF') line = line.substring(1); // strip BOM
                            buffer.append('\n');
                            buffer.append(line);
                        }
                        br.close();
                        String id = fileName;
                        int pos = id.lastIndexOf('.');
                        if (pos >= 0) id = id.substring(0, pos);
                        frame.setTransliterator(buffer.toString(), id);
                    } else if (choice == TEST_FILE) {
                        genTestFile(f, frame.translit);
                    }
                } catch (Exception e2) {
                    e2.printStackTrace();
                    System.out.println("Problem opening/reading: " + fileDirectory + ", " + fileName);
                }
            }
            fileDialog.dispose();
        }
    }
    

    boolean transliterateTyping = true;
    Transliterator fromHex = Transliterator.getInstance("Hex-Any");
    InfoDialog helpDialog;
    InfoDialog hexDialog;
    InfoDialog compoundDialog;
    InfoDialog rulesDialog;
    TextField ruleId;
    MenuItem convertSelectionItem = null;
    MenuItem swapSelectionItem = null;
    MenuItem convertTypingItem = null;
    Menu historyMenu;
    Map historyMap = new HashMap();
    Set historySet = new TreeSet(new Comparator() {
            public int compare(Object a, Object b) {
                MenuItem aa = (MenuItem)a;
                MenuItem bb = (MenuItem)b;
                return aa.getLabel().compareTo(bb.getLabel());
            }
        });
        
    // ADD Factory since otherwise getInverse blows out
    static class DummyFactory implements Transliterator.Factory {
        static DummyFactory singleton = new DummyFactory();
        static HashMap m = new HashMap();

        // Since Transliterators are immutable, we don't have to clone on set & get
        static void add(String ID, Transliterator t) {
            m.put(ID, t);
            System.out.println("Registering: " + ID + ", " + t.toRules(true));
            Transliterator.registerFactory(ID, singleton);
        }
        public Transliterator getInstance(String ID) {
            return (Transliterator) m.get(ID);
        }
    }
    
    static void printBreaks(int num, String testSource, BreakIterator bi) {
        String result = "";
        int lastPos = 0;
    	while (true) {
    	    int pos = bi.next();
    	    if (pos == bi.DONE) break;
    	    result += testSource.substring(lastPos, pos) + "&";
    	    lastPos = pos;
    	    System.out.println(pos);
    	}
    	System.out.println("Test" + num + ": " + result);
    }
    
    static void printIteration(int num, String testSource, CharacterIterator ci) {
        String result = "";
    	while (true) {
    	    char ch = ci.next();
    	    if (ch == ci.DONE) break;
    	    result += ch + "(" + ci.getIndex() + ")";
    	}
    	System.out.println("Test" + num + ": " + result);
    }
    
    static void printSources() {
        String[] list = {"Latin-ThaiLogical", "ThaiLogical-Latin", "Thai-ThaiLogical", "ThaiLogical-Thai"};
        UnicodeSet all = new UnicodeSet();
        for (int i = 0; i < list.length; ++i) {
            Transliterator tr = Transliterator.getInstance(list[i]);
            UnicodeSet src = tr.getSourceSet();
            System.out.println(list[i] + ": " + src.toPattern(true));
            all.addAll(src);
        }
        System.out.println("All: " + all.toPattern(true));
        UnicodeSet rem = new UnicodeSet("[[:latin:][:thai:]]");
        System.out.println("missing from [:latin:][:thai:]: " + all.removeAll(rem).toPattern(true));
    }
    
    static void genTestFile(File sourceFile, Transliterator translit) {
        try {
            
            System.out.println("Reading: " + sourceFile.getCanonicalPath());
            BufferedReader in = new BufferedReader(
                new InputStreamReader(
                    new FileInputStream(sourceFile), "UTF-8"));
            String targetFile = sourceFile.getCanonicalPath();
            int dotPos = targetFile.lastIndexOf('.');
            if (dotPos >= 0) targetFile = targetFile.substring(0,dotPos);
            
            File outFile = new File(targetFile + ".html");
            System.out.println("Writing: " + outFile.getCanonicalPath());
            
            PrintWriter out = new PrintWriter(
                new BufferedWriter(
                    new OutputStreamWriter(
                        new FileOutputStream(outFile), "UTF-8")));
            String direction = "";
            String id = translit.getID();
            if (id.indexOf("Arabic") >= 0 || id.indexOf("Hebrew") >= 0) {
                direction = " direction: rtl;";
            }
            out.println("<head><meta http-equiv='Content-Type' content='text/html; charset=utf-8'>");
            out.println("<style><!--");
            out.println("td, th       { vertical-align: top; border: 1px solid black }");
            out.println("td.s       { background-color: #EEEEEE;" + direction + " }");
            out.println("td.r       { background-color: #CCCCCC;" + direction + " }");
            out.println("span.d       { background-color: #FF0000 }");
            out.println("body         { font-family: 'Arial Unicode MS', 'Lucida Sans Unicode', Arial, sans-serif; margin: 5 }");
            out.println("--></style>");
            out.println("<title>" + id + " Transliteration Check</title></head>");
            out.println("<body bgcolor='#FFFFFF'><table>");
            //out.println("<tr><th width='33%'>Thai</th><th width='33%'>Latin</th><th width='33%'>Thai</th></tr>");
  
            Transliterator tl = translit;
            Transliterator lt = tl.getInverse();
            
            Transliterator title = Transliterator.getInstance("title");
            Transliterator upper = Transliterator.getInstance("upper");
            Transliterator ltFilter = tl.getInverse();
            ltFilter.setFilter(new UnicodeSet("[:^Lu:]"));
            Transliterator tlFilter = lt.getInverse();
            tlFilter.setFilter(new UnicodeSet("[:^Lu:]"));
            
            //Transliterator.getInstance("[:^Lu:]" +  lt.getID());
            
            BreakIterator sentenceBreak = BreakIterator.getSentenceInstance();
            
            int NONE = 0, TITLEWORD = 1;
            int titleSetting = NONE;
            boolean upperfilter = false;
            boolean first = true;
            while (true) {
                String line = in.readLine();
                if (line == null) break;
                line = line.trim();
                if (line.length() == 0) continue;
                if (line.charAt(0) == '\uFEFF') line = line.substring(1); // remove BOM
                
                if (line.charAt(0) == '#') continue; // comments
                
                if (line.equals("@TITLECASE@")) {
                    titleSetting = TITLEWORD;
                    continue;
                } else if (line.equals("@UPPERFILTER@")) {
                    upperfilter = true;
                    continue;
                }
                
                if (upperfilter) {
                    line = upper.transliterate(line);
                }
                
                sentenceBreak.setText(line);
                int start = 0;
                while (true) {
                    int end = sentenceBreak.next();
                    if (end == sentenceBreak.DONE) break;
                    String sentence = line.substring(start, end);
                    end = start;
                    
                    String latin;
                    if (upperfilter) {
                        latin = tlFilter.transliterate(sentence);
                    } else {
                        latin = tl.transliterate(sentence);
                    }                       
                    String latinShow = latin;
                    if (titleSetting == TITLEWORD) {
                        latinShow = title.transliterate(latin);
                    } else {
                        latinShow = titlecaseFirstWord(latinShow);
                    }
                    String reverse;
                    if (upperfilter) {
                        reverse = ltFilter.transliterate(latin);
                    } else {
                        reverse = lt.transliterate(latin);
                    }
                    
                    String NFCsentence = Normalizer.normalize(sentence, Normalizer.NFC);
                    
                    if (!reverse.equals(NFCsentence)) {
                        int minLen = reverse.length();
                        if (minLen > sentence.length()) minLen = sentence.length();
                        int i;
                        for (i = 0; i < minLen; ++i) {
                            if (reverse.charAt(i) != sentence.charAt(i)) break;
                        }
                        reverse = reverse.substring(0,i) + "<span class='d'>" + reverse.substring(i) + "</span>";
                        sentence = sentence.substring(0,i) + "<span class='d'>" + sentence.substring(i) + "</span>";
                        out.println("<tr><td class='s'" + (first ? " width='50%'>" : ">") + sentence 
                            + "</td><td rowSpan='2'>" + latinShow
                            + "</td></tr><tr><td class='r'>" + reverse
                            + "</td></tr><tr><td></td></tr>");
                    } else {
                        out.println("<tr><td class='s'" + (first ? " width='50%'>" : ">") + sentence 
                            + "</td><td>" + latinShow
                            + "</td></tr><tr><td></td></tr>");
                    }
                    first = false;
                }
            }
            int dashPos = id.indexOf('-');
            int slashPos = id.indexOf('/');
            if (slashPos < 0) slashPos = id.length();
            UnicodeSet sourceSuper = null;
            try {
                sourceSuper = new UnicodeSet("[:" + id.substring(0,dashPos) + ":]");
            } catch (Exception e) {}
            
            UnicodeSet targetSuper = null;
            try {
                targetSuper = new UnicodeSet("[:" + id.substring(dashPos+1, slashPos) + ":]");
            } catch (Exception e) {}
            
            out.println("</table><ul>");
            out.println("<p><b>NFD</b></p>");
            out.println("<li>Source Set:<ul><li>" + toPattern(closeUnicodeSet(translit.getSourceSet(), Normalizer.NFD, true), sourceSuper) + "</li></ul></li>");
            out.println("<li>Reverse Target Set:<ul><li>" + toPattern(closeUnicodeSet(lt.getTargetSet(), Normalizer.NFD, true), sourceSuper) + "</li></ul></li>");
            out.println("<li>Target Set:<ul><li>" + toPattern(closeUnicodeSet(translit.getTargetSet(), Normalizer.NFD, true), targetSuper) + "</li></ul></li>");
            out.println("<li>Reverse Source Set:<ul><li>" + toPattern(closeUnicodeSet(lt.getSourceSet(), Normalizer.NFD, true), targetSuper) + "</li></ul></li>");
            out.println("<p><b>NFKD</b></p>");
            out.println("<li>Source Set:<ul><li>" + toPattern(closeUnicodeSet(translit.getSourceSet(), Normalizer.NFKD, true), sourceSuper) + "</li></ul></li>");
            out.println("<li>Reverse Target Set:<ul><li>" + toPattern(closeUnicodeSet(lt.getTargetSet(), Normalizer.NFKD, true), sourceSuper) + "</li></ul></li>");
            out.println("<li>Target Set:<ul><li>" + toPattern(closeUnicodeSet(translit.getTargetSet(), Normalizer.NFKD, true), targetSuper) + "</li></ul></li>");
            out.println("<li>Reverse Source Set:<ul><li>" + toPattern(closeUnicodeSet(lt.getSourceSet(), Normalizer.NFKD, true), targetSuper) + "</li></ul></li>");
            out.println("</ul></body>");
            out.close();
            System.out.println("Done Writing");
        } catch (Exception e) {
            e.printStackTrace();
        }
    }
    
    
    static UnicodeSet closeUnicodeSet(UnicodeSet source, Normalizer.Mode mode, boolean caseToo) {
        UnicodeSetIterator it = new UnicodeSetIterator(source);
        UnicodeSet additions = new UnicodeSet(); // to avoid messing up iterator
        int cp;
        
        // First add all case equivalents
        if (caseToo) {
            while (it.next()) {
                cp = it.codepoint;
                if (cp == it.IS_STRING) continue;
                int type = UCharacter.getType(cp);
                if (type == Character.UPPERCASE_LETTER || type == Character.LOWERCASE_LETTER || type == Character.TITLECASE_LETTER) {
                    additions.add(UCharacter.toLowerCase(UTF16.valueOf(cp)));
                    additions.add(UCharacter.toUpperCase(UTF16.valueOf(cp)));
                }
            }
            source.addAll(additions);
            additions.clear();
        }
       
        // Now add all decompositions of characters in source
        it.reset(source);
        while (it.next()) {
            cp = it.codepoint;
            if (cp == it.IS_STRING) continue;
            if (Normalizer.isNormalized(cp, mode)) continue;
            String decomp = Normalizer.normalize(cp, mode);
            additions.add(decomp);
        }
        source.addAll(additions);
        
        // Now add any other character that decomposes to a character in source
        for (cp = 0; cp < 0x10FFFF; ++cp) {
            if (!UCharacter.isDefined(cp)) continue;
            if (Normalizer.isNormalized(cp, mode)) continue;
            if (source.contains(cp)) continue;
            
            String decomp = Normalizer.normalize(cp, mode);
            if (source.containsAll(decomp)) {
                System.out.println("Adding: " + Integer.toString(cp,16) + " " + UCharacter.getName(cp));
                source.add(cp);
            }
        }
        // For completeness, later we should add the canonical closure of all strings in source
        return source;
    }
    
    static String toPattern(UnicodeSet source, UnicodeSet superset) {
        if (superset != null) {
            source.removeAll(superset);
            return "[" + superset.toPattern(true) + " " + source.toPattern(true) + "]";
        }
        return source.toPattern(true);
    }
    
    static BreakIterator bi = BreakIterator.getWordInstance();
    
    static String titlecaseFirstWord(String line) {
        // search for first word with letters. If the first letter is lower, then titlecase it.
        bi.setText(line);
        int start = 0;
        while (true) {
            int end = bi.next();
            if (end == bi.DONE) break;
            int firstLetterType = getFirstLetterType(line, start, end);
            if (firstLetterType != Character.UNASSIGNED) {
                if (firstLetterType != Character.LOWERCASE_LETTER) break;
                line = line.substring(0, start) 
                    + UCharacter.toTitleCase(line.substring(start, end), bi)
                    + line.substring(end);
                break;
            }
            end = start;
        }
        return line;
    }
    
    static final int LETTER_MASK = 
          (1<<Character.UPPERCASE_LETTER)
        | (1<<Character.LOWERCASE_LETTER)
        | (1<<Character.TITLECASE_LETTER)
        | (1<<Character.MODIFIER_LETTER)
        | (1<<Character.OTHER_LETTER)
        ;
    
    static int getFirstLetterType(String line, int start, int end) {
        int cp;
        for (int i = start; i < end; i += UTF16.getCharCount(cp)) {
            cp = UTF16.charAt(line, i);
            int type = UCharacter.getType(cp);
            if (((1<<type) & LETTER_MASK) != 0) return type;
        }
        return Character.UNASSIGNED;
    }
    
    static void printNames(UnicodeSet s, String targetFile) {
        try {
            File outFile = new File(targetFile);
            System.out.println("Writing: " + outFile.getCanonicalPath());
                
            PrintWriter out = new PrintWriter(
                new BufferedWriter(
                    new OutputStreamWriter(
                        new FileOutputStream(outFile), "UTF-8")));
            UnicodeSet main = new UnicodeSet();
            
            UnicodeSet others = new UnicodeSet();
            UnicodeSetIterator it = new UnicodeSetIterator(s);
            while (it.next()) {
                if (!UCharacter.isDefined(it.codepoint)) continue;
                if (!Normalizer.isNormalized(it.codepoint, Normalizer.NFD)) {
                    String decomp = Normalizer.normalize(it.codepoint, Normalizer.NFD);
                    others.addAll(decomp);
                    continue;
                }
                out.println(" " + UTF16.valueOf(it.codepoint) + " <> XXX # " + UCharacter.getName(it.codepoint));
                main.add(it.codepoint);
            }
            
            if (others.size() != 0) {
                out.println("Decomposed characters found above: ");
                others.removeAll(main);
                it.reset(others);
                while (it.next()) {
                    out.println(" " + UTF16.valueOf(it.codepoint) + " <> XXX # " + UCharacter.getName(it.codepoint));
                }
            }
            
            out.close();
            System.out.println("Done Writing");
        } catch (Exception e) {
            e.printStackTrace();
        }
    }
    
    
    static {
        
        printNames(new UnicodeSet("[\u0600-\u06FF]"), "Arabic-Latin.txt");
        
        
    	if (false) {
        BreakTransliterator.register();
        
    	BreakTransliterator testTrans = new BreakTransliterator("Any-XXX", null, null, "$");
    	String testSource = "The Quick:   Brown fox--jumped.";
    	BreakIterator bi = testTrans.getBreakIterator();
        bi.setText(new StringCharacterIterator(testSource));
        printBreaks(0, testSource, bi);
        //bi.setText(UCharacterIterator.getInstance(testSource));
        //printBreaks(1, testSource, bi);
        
        printIteration(2, testSource, new StringCharacterIterator(testSource));
        //printIteration(3, testSource, UCharacterIterator.getInstance(testSource));
        
    	
    	
    	String test = testTrans.transliterate(testSource);
    	System.out.println("Test3: " + test);
    	DummyFactory.add(testTrans.getID(), testTrans);
    	
    	// AnyTransliterator.ScriptRunIterator.registerAnyToScript();
    	
    	AnyTransliterator at = new AnyTransliterator("Greek", null);
    	at.transliterate("(cat,\u03b1,\u0915)");
    	DummyFactory.add(at.getID(), at);
    	
    	at = new AnyTransliterator("Devanagari", null);
    	at.transliterate("(cat,\u03b1,\u0915)");
    	DummyFactory.add(at.getID(), at);
    	
    	at = new AnyTransliterator("Latin", null);
    	at.transliterate("(cat,\u03b1,\u0915)");
    	DummyFactory.add(at.getID(), at);
    	
        DummyFactory.add("Any-gif", Transliterator.createFromRules("gif", "'\\'u(..)(..) > '<img src=\"http://www.unicode.org/gifs/24/' $1 '/U' $1$2 '.gif\">';", Transliterator.FORWARD));    	
        DummyFactory.add("gif-Any", Transliterator.getInstance("Any-Null"));    	

        DummyFactory.add("Any-RemoveCurly", Transliterator.createFromRules("RemoveCurly", "[\\{\\}] > ;", Transliterator.FORWARD));    	
        DummyFactory.add("RemoveCurly-Any", Transliterator.getInstance("Any-Null"));
        
        System.out.println("Trying &hex");
        Transliterator t = Transliterator.createFromRules("hex2", "(.) > &hex($1);", Transliterator.FORWARD);
        System.out.println("Registering");
        DummyFactory.add("Any-hex2", t);    	
        
        System.out.println("Trying &gif");
        t = Transliterator.createFromRules("gif2", "(.) > &any-gif($1);", Transliterator.FORWARD);
        System.out.println("Registering");
        DummyFactory.add("Any-gif2", t);    
        }
    }
    
    
    void setTransliterator(String name, String id) {
        if (DEBUG) System.out.println("Got: " + name);
        if (id == null) {
        	translit = Transliterator.getInstance(name);
        } else {
            int pos = id.indexOf('-');
            String reverseId = "";
            if (pos < 0) {
            	reverseId = id + "-Any";
            	id = "Any-" + id;
            } else {
            	reverseId = id.substring(pos+1) + "-" + id.substring(0,pos);
            }
            
        	
        	translit = Transliterator.createFromRules(id, name, Transliterator.FORWARD);
        	if (DEBUG) {
        	    System.out.println("***Forward Rules");
        	    System.out.println(((RuleBasedTransliterator)translit).toRules(true));
        	    System.out.println("***Source Set");
        	    System.out.println(translit.getSourceSet().toPattern(true));
        	}
        	    System.out.println("***Target Set");
        	    UnicodeSet target = translit.getTargetSet();
        	    System.out.println(target.toPattern(true));
        	    UnicodeSet rest = new UnicodeSet("[a-z]").removeAll(target);
        	    System.out.println("***ASCII - Target Set");
        	    System.out.println(rest.toPattern(true));
        	    
            DummyFactory.add(id, translit);
            
        	Transliterator translit2 = Transliterator.createFromRules(reverseId, name, Transliterator.REVERSE);
        	if (DEBUG) {
        	    System.out.println("***Backward Rules");
        	    System.out.println(((RuleBasedTransliterator)translit2).toRules(true));
        	}
            DummyFactory.add(reverseId, translit2);
            
            Transliterator rev = translit.getInverse();
        	if (DEBUG) System.out.println("***Inverse Rules");
        	if (DEBUG) System.out.println(((RuleBasedTransliterator)rev).toRules(true));
            
        }
        text.flush();
        text.setTransliterator(translit);
        convertSelectionItem.setLabel(Transliterator.getDisplayName(translit.getID()));
        
        addHistory(translit);
        
        Transliterator inv;
        try {
        	inv = translit.getInverse();
        } catch (Exception ex) {
        	inv = null;
        }
        if (inv != null) {
            addHistory(inv);
            swapSelectionItem.setEnabled(true);
        } else {
            swapSelectionItem.setEnabled(false);
        }
    }
    
    void addHistory(Transliterator translit) {
        String name = translit.getID();
        MenuItem cmi = (MenuItem) historyMap.get(name);
        if (cmi == null) {
            cmi = new MenuItem(translit.getDisplayName(name));
            cmi.addActionListener(new TransliterationListener(name));
            historyMap.put(name, cmi);
            historySet.add(cmi);
            historyMenu.removeAll();
            Iterator it = historySet.iterator();
            while (it.hasNext()) {
                historyMenu.add((MenuItem)it.next());
            }
        }
    }
    
    class TransliterationListener implements ActionListener, ItemListener {
        String name;
        public TransliterationListener(String name) {
            this.name = name;
        }
        public void actionPerformed(ActionEvent e) {
            setTransliterator(name, null);
        }
        public void itemStateChanged(ItemEvent e) {
            if (e.getStateChange() == e.SELECTED) {
                setTransliterator(name, null);
            } else {
                setTransliterator("Any-Null", null);
            }
        }
    }
    
    class FontActionListener implements ActionListener {
        String name;
        public FontActionListener(String name) {
            this.name = name;
        }
        public void actionPerformed(ActionEvent e) {
            if (DEBUG) System.out.println("Font: " + name);
            fontName = name;
            text.setFont(new Font(fontName, Font.PLAIN, fontSize));
        }
    }
    
    class SizeActionListener implements ActionListener {
        int size;
        public SizeActionListener(int size) {
            this.size = size;
        }
        public void actionPerformed(ActionEvent e) {
            if (DEBUG) System.out.println("Size: " + size);
            fontSize = size;
            text.setFont(new Font(fontName, Font.PLAIN, fontSize));
        }
    }
    
    Set add(Set s, Enumeration enum) {
        while(enum.hasMoreElements()) {
            s.add(enum.nextElement());
        }
        return s;
    }

    /**
     * Get a sorted list of the system transliterators.
     */
     /*
    private static Vector getSystemTransliteratorNames() {
        Vector v = new Vector();
        for (Enumeration e=Transliterator.getAvailableIDs();
             e.hasMoreElements(); ) {
            v.addElement(e.nextElement());
        }
        // Insertion sort, O(n^2) acceptable for small n
        for (int i=0; i<(v.size()-1); ++i) {
            String a = (String) v.elementAt(i);
            for (int j=i+1; j<v.size(); ++j) {
                String b = (String) v.elementAt(j);
                if (a.compareTo(b) > 0) {
                    v.setElementAt(b, i);
                    v.setElementAt(a, j);
                    a = b;
                }
            }
        }
        return v;
    }
    */

/*
    private void setNoTransliterator() {
        translitItem = noTranslitItem;
        noTranslitItem.setState(true);
        handleSetTransliterator(noTranslitItem.getLabel());
        compound = false;
        for (int i=0; i<translitMenu.getItemCount(); ++i) {
            MenuItem it = translitMenu.getItem(i);
            if (it != noTranslitItem && it instanceof CheckboxMenuItem) {
                ((CheckboxMenuItem) it).setState(false);
            }
        }
    }
*/
/*
    private void handleAddToCompound(String name) {
        if (compoundCount < MAX_COMPOUND) {
            compoundTranslit[compoundCount] = decodeTranslitItem(name);
            ++compoundCount;
            Transliterator t[] = new Transliterator[compoundCount];
            System.arraycopy(compoundTranslit, 0, t, 0, compoundCount);
            translit = new CompoundTransliterator(t);
            text.setTransliterator(translit);
        }
    }
*/
/*
    private void handleSetTransliterator(String name) {
        translit = decodeTranslitItem(name);
        text.setTransliterator(translit);
    }
    */

    /**
     * Decode a menu item that looks like <translit name>.
     */
     /*
    private static Transliterator decodeTranslitItem(String name) {
        return (name.equals(NO_TRANSLITERATOR))
            ? null : Transliterator.getInstance(name);
    }
    */

    private void handleBatchTransliterate(Transliterator translit) {
        if (translit == null) {
            return;
        }

        int start = text.getSelectionStart();
        int end = text.getSelectionEnd();
        ReplaceableString s =
            new ReplaceableString(text.getText().substring(start, end));

        StringBuffer log = null;
        if (DEBUG) {
            log = new StringBuffer();
            log.append('"' + s.toString() + "\" (start " + start +
                       ", end " + end + ") -> \"");
        }

        translit.transliterate(s);
        String str = s.toString();

        if (DEBUG) {
            log.append(str + "\"");
            System.out.println("Batch " + translit.getID() + ": " + log.toString());
        }

        text.replaceRange(str, start, end);
        text.select(start, start + str.length());
    }

    private void handleClose() {
        helpDialog.dispose();
        dispose();
    }
    
    /*
    class InfoDialog extends Dialog {
        protected Button button;
        protected TextArea area;
        protected Dialog me;
        protected Panel bottom;
        
        public TextArea getArea() {
            return area;
        }
        
        public Panel getBottom() {
            return bottom;
        }
        
        InfoDialog(Frame parent, String title, String label, String message) {
            super(parent, title, false);
            me = this;
            this.setLayout(new BorderLayout());
            if (label.length() != 0) {
                this.add("North", new Label(label));
            }
            
            area = new TextArea(message, 8, 80, TextArea.SCROLLBARS_VERTICAL_ONLY);
            this.add("Center", area);
            
            button = new Button("Hide");
            button.addActionListener(new ActionListener() {
                public void actionPerformed(ActionEvent e) {
                    me.hide();
                }
            });
            bottom = new Panel();
            bottom.setLayout(new FlowLayout(FlowLayout.CENTER, 0, 0));
            bottom.add(button);
            this.add("South", bottom);
            this.pack();
            addWindowListener(new WindowAdapter() {
                public void windowClosing(WindowEvent e) {
                    me.hide();
                }
            });
        }
    }
    */
}
