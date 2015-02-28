/*
 *******************************************************************************
 * Copyright (C) 2014, International Business Machines Corporation and         *
 * others. All Rights Reserved.                                                *
 *******************************************************************************
 */
package com.ibm.icu.dev.test.stringprep;

import java.io.BufferedReader;
import java.io.File;
import java.io.FileInputStream;
import java.io.IOException;
import java.io.InputStreamReader;
import java.text.ParsePosition;
import java.util.Arrays;
import java.util.List;
import java.util.regex.Matcher;
import java.util.regex.Pattern;

import com.ibm.icu.dev.test.TestFmwk;
import com.ibm.icu.text.Linkifier;
import com.ibm.icu.text.Linkifier.Status;
import com.ibm.icu.text.Linkifier.UrlScanner;

/**
 * Finds possible links (URLs, mailto's, etc) within text.
 * 
 * @author markdavis
 */
public class TestLinkifier extends TestFmwk {
    /**
     * 
     */
    private static final String TEST_LINKIFIER_TXT = "TestLinkifier.txt";

    public static void main(String[] args) {
        new TestLinkifier().run(args);
    }

    public void TestPercentEscaping() {
        checkEscape("%", "%", 1);
        checkEscape("%x", "%", 1);
        checkEscape("%6", "%", 1);
        checkEscape("%6x", "%", 1);
        checkEscape("%61%x", "a%", 4);
        checkEscape("%61%61", "aa", 6);
        checkEscape("%61%61x", "aa", 6);
    }

    private void checkEscape(String source, String expectedString, int expectedEaten) {
        ParsePosition pos = new ParsePosition(0);
        String s = Linkifier.getPercentEscaped(source, pos, Linkifier.UTF8_CHARSET);
        assertEquals(source, expectedString, s);
        assertEquals(source, expectedEaten, pos.getIndex());
    }

    public void TestSamples() throws IOException {
        UrlScanner linkifier = new UrlScanner(TLDS);
        Matcher commentFinder = Pattern.compile("(^|\\s)#").matcher("");
        File file = new File(TEST_LINKIFIER_TXT);
        logln("Reading from: " + file.getCanonicalPath());
        BufferedReader in = new BufferedReader(
                new InputStreamReader(
                        new FileInputStream(file), "UTF-8"));
        for (int lineNumber = 1; ; ++lineNumber) {
            String line = in.readLine();
            if (line == null) {
                break;
            }
            String comment;
            String test;
            if (commentFinder.reset(line).find()) {
                comment = line.substring(commentFinder.end()).trim();
                test = line.substring(0, commentFinder.start()).trim();
            } else {
                comment = "";
                test = line.trim();
            }
            if (isVerbose() && !comment.isEmpty()) {
                logln(comment);
            }
            if (test.isEmpty()) {
                continue;
            }
            String[] parts = test.split("\t");
            Status expected = parts.length < 2 ? Status.WELL_FORMED : Status.valueOf(parts[1]);
            String source = "(" + parts[0] + ")";
            logln(source);
            ParsePosition pos = new ParsePosition(0);
            Status foundOne = Status.ILL_FORMED;
            int end;
            for (int start = 0; start < source.length(); start = end) {
                pos.setIndex(start);
                Status result = linkifier.scan(source, pos);
                end = pos.getIndex();
                if (result != Status.WELL_FORMED) {
                    end = start + 1;
                    continue;
                }
                // the test is set up so that the first and last character should be skipped
                final int fromEnd = source.length() - end;
                if (start != 1 || fromEnd != 1 && expected == Status.WELL_FORMED) {
                    errln(lineNumber + "\t" + showLine(source, start, end, fromEnd));
                } else {
                    logln(lineNumber + "\t" + showLine(source, start, end, fromEnd));
                }
                foundOne = Status.WELL_FORMED;
            }
            if (foundOne != expected) {
                assertEquals(lineNumber + "\t" + test, expected, foundOne);
            }
        }
        in.close();
    }

    private String showLine(String source, int start, int end, final int fromEnd) {
        return end + "\t" + start + "\t" + fromEnd
                + "\t«" + source.substring(0, start)
                + "❴❴❴" + source.substring(start, end)
                + "❵❵❵" + source.substring(end, source.length())
                + "»";
    }

    static final List<String> TLDS = Arrays.asList("abogado", "ac", "academy", "accountants", "active", "actor", "ad",
            "adult", "ae", "aero", "af", "ag", "agency", "ai", "airforce", "al", "allfinanz", "alsace", "am", "an",
            "android", "ao", "aq", "aquarelle", "ar", "archi", "army", "arpa", "as", "asia", "associates", "at",
            "attorney", "au", "auction", "audio", "autos", "aw", "ax", "axa", "az", "ba", "band", "bar", "bargains",
            "bayern", "bb", "bd", "be", "beer", "berlin", "best", "bf", "bg", "bh", "bi", "bid", "bike", "bio", "biz",
            "bj", "bl", "black", "blackfriday", "bloomberg", "blue", "bm", "bmw", "bn", "bnpparibas", "bo", "boo",
            "boutique", "bq", "br", "brussels", "bs", "bt", "budapest", "build", "builders", "business", "buzz", "bv",
            "bw", "by", "bz", "bzh", "ca", "cab", "cal", "camera", "camp", "cancerresearch", "capetown", "capital",
            "caravan", "cards", "care", "career", "careers", "cartier", "casa", "cash", "cat", "catering", "cc", "cd",
            "center", "ceo", "cern", "cf", "cg", "ch", "channel", "cheap", "christmas", "chrome", "church", "ci",
            "citic", "city", "ck", "cl", "claims", "cleaning", "click", "clinic", "clothing", "club", "cm", "cn", "co",
            "coach", "codes", "coffee", "college", "cologne", "com", "community", "company", "computer", "condos",
            "construction", "consulting", "contractors", "cooking", "cool", "coop", "country", "cr", "credit",
            "creditcard", "cricket", "crs", "cruises", "cu", "cuisinella", "cv", "cw", "cx", "cy", "cymru", "cz",
            "dad", "dance", "dating", "day", "de", "deals", "degree", "delivery", "democrat", "dental", "dentist",
            "desi", "dev", "diamonds", "diet", "digital", "direct", "directory", "discount", "dj", "dk", "dm", "dnp",
            "do", "docs", "domains", "doosan", "durban", "dvag", "dz", "eat", "ec", "edu", "education", "ee", "eg",
            "eh", "email", "emerck", "energy", "engineer", "engineering", "enterprises", "equipment", "er", "es",
            "esq", "estate", "et", "eu", "eurovision", "eus", "events", "everbank", "exchange", "expert", "exposed",
            "fail", "farm", "fashion", "feedback", "fi", "finance", "financial", "firmdale", "fish", "fishing",
            "fitness", "fj", "fk", "flights", "florist", "flsmidth", "fly", "fm", "fo", "foo", "forsale", "foundation",
            "fr", "frl", "frogans", "fund", "furniture", "futbol", "ga", "gal", "gallery", "garden", "gb", "gbiz",
            "gd", "ge", "gent", "gf", "gg", "gh", "gi", "gift", "gifts", "gives", "gl", "glass", "gle", "global",
            "globo", "gm", "gmail", "gmo", "gmx", "gn", "google", "gop", "gov", "gp", "gq", "gr", "graphics", "gratis",
            "green", "gripe", "gs", "gt", "gu", "guide", "guitars", "guru", "gw", "gy", "hamburg", "haus",
            "healthcare", "help", "here", "hiphop", "hiv", "hk", "hm", "hn", "holdings", "holiday", "homes", "horse",
            "host", "hosting", "house", "how", "hr", "ht", "hu", "ibm", "id", "ie", "il", "im", "immo", "immobilien",
            "in", "industries", "info", "ing", "ink", "institute", "insure", "int", "international", "investments",
            "io", "iq", "ir", "irish", "is", "it", "iwc", "je", "jetzt", "jm", "jo", "jobs", "joburg", "jp", "juegos",
            "kaufen", "ke", "kg", "kh", "ki", "kim", "kitchen", "kiwi", "km", "kn", "koeln", "kp", "kr", "krd", "kred",
            "kw", "ky", "kz", "la", "lacaixa", "land", "latrobe", "lawyer", "lb", "lc", "lds", "lease", "legal",
            "lgbt", "li", "lidl", "life", "lighting", "limited", "limo", "link", "lk", "loans", "london", "lotto",
            "lr", "ls", "lt", "ltda", "lu", "luxe", "luxury", "lv", "ly", "ma", "madrid", "maison", "management",
            "mango", "market", "marketing", "mc", "md", "me", "media", "meet", "melbourne", "meme", "memorial", "menu",
            "mf", "mg", "mh", "miami", "mil", "mini", "mk", "ml", "mm", "mn", "mo", "mobi", "moda", "moe", "monash",
            "money", "mormon", "mortgage", "moscow", "motorcycles", "mov", "mp", "mq", "mr", "ms", "mt", "mu",
            "museum", "mv", "mw", "mx", "my", "mz", "na", "nagoya", "name", "navy", "nc", "ne", "net", "network",
            "neustar", "new", "nexus", "nf", "ng", "ngo", "nhk", "ni", "ninja", "nl", "no", "np", "nr", "nra", "nrw",
            "nu", "nyc", "nz", "okinawa", "om", "ong", "onl", "ooo", "org", "organic", "osaka", "otsuka", "ovh", "pa",
            "paris", "partners", "parts", "party", "pe", "pf", "pg", "ph", "pharmacy", "photo", "photography",
            "photos", "physio", "pics", "pictures", "pink", "pizza", "pk", "pl", "place", "plumbing", "pm", "pn",
            "pohl", "poker", "porn", "post", "pr", "praxi", "press", "pro", "prod", "productions", "prof",
            "properties", "property", "ps", "pt", "pub", "pw", "py", "qa", "qpon", "quebec", "re", "realtor",
            "recipes", "red", "rehab", "reise", "reisen", "reit", "ren", "rentals", "repair", "report", "republican",
            "rest", "restaurant", "reviews", "rich", "rio", "rip", "ro", "rocks", "rodeo", "rs", "rsvp", "ru", "ruhr",
            "rw", "ryukyu", "sa", "saarland", "samsung", "sarl", "sb", "sc", "sca", "scb", "schmidt", "schule",
            "schwarz", "science", "scot", "sd", "se", "services", "sew", "sexy", "sg", "sh", "shiksha", "shoes", "si",
            "singles", "sj", "sk", "sky", "sl", "sm", "sn", "so", "social", "software", "sohu", "solar", "solutions",
            "soy", "space", "spiegel", "sr", "ss", "st", "su", "supplies", "supply", "support", "surf", "surgery",
            "suzuki", "sv", "sx", "sy", "sydney", "systems", "sz", "taipei", "tatar", "tattoo", "tax", "tc", "td",
            "technology", "tel", "tf", "tg", "th", "tienda", "tips", "tires", "tirol", "tj", "tk", "tl", "tm", "tn",
            "to", "today", "tokyo", "tools", "top", "town", "toys", "tp", "tr", "trade", "training", "travel", "trust",
            "tt", "tui", "tv", "tw", "tz", "ua", "ug", "uk", "um", "university", "uno", "uol", "us", "uy", "uz", "va",
            "vacations", "vc", "ve", "vegas", "ventures", "versicherung", "vet", "vg", "vi", "viajes", "villas",
            "vision", "vlaanderen", "vn", "vodka", "vote", "voting", "voto", "voyage", "vu", "wales", "wang", "watch",
            "webcam", "website", "wed", "wedding", "wf", "whoswho", "wien", "wiki", "williamhill", "wme", "work",
            "works", "world", "ws", "wtc", "wtf", "测试", "परीक्षा", "佛山", "集团", "在线", "한국", "ভারত", "八卦", "موقع",
            "বাংলা", "公益", "公司", "移动", "我爱你", "москва", "испытание", "қаз", "онлайн", "сайт", "срб", "бел", "테스트",
            "орг", "삼성", "சிங்கப்பூர்", "商标", "商店", "商城", "дети", "мкд", "טעסט", "中文网", "中信", "中国", "中國", "谷歌",
            "భారత్", "ලංකා", "測試", "ભારત", "भारत", "آزمایشی", "பரிட்சை", "网店", "संगठन", "网络", "укр", "香港", "δοκιμή",
            "إختبار", "台湾", "台灣", "手机", "мон", "الجزائر", "عمان", "ایران", "امارات", "بازار", "پاکستان", "الاردن",
            "بھارت", "المغرب", "السعودية", "سودان", "عراق", "مليسيا", "شبكة", "გე", "机构", "组织机构", "ไทย", "سورية",
            "рус", "рф", "تونس", "みんな", "グーグル", "世界", "ਭਾਰਤ", "网址", "游戏", "vermögensberater", "vermögensberatung",
            "企业", "مصر", "قطر", "广东", "இலங்கை", "இந்தியா", "հայ", "新加坡", "فلسطين", "テスト", "政务", "xxx", "xyz", "yachts",
            "yandex", "ye", "yoga", "yokohama", "youtube", "yt", "za", "zip", "zm", "zone", "zw");
}
