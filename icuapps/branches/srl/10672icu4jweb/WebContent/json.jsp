<%@page import="com.ibm.icu.text.LocaleDisplayNames.DialectHandling"%>
<%@ page language="java" contentType="application/json; charset=UTF-8"
    pageEncoding="UTF-8"
    import="org.json.simple.JSONObject,com.ibm.icu.util.*,com.ibm.icu.text.*,com.ibm.icu.lang.*" %><%!
    	    enum What { none, locales, verinfo, acceptlang } ;
 			@SuppressWarnings("unchecked") 
 %><%
	JSONObject o = new JSONObject();
    String path[] = request.getPathInfo().substring(1).split("/");
    What what = path.length>0?What.valueOf(path[0]):What.none;
    o.put("what",what.toString());    
    ULocale dloc = path.length>1?ULocale.forLanguageTag(path[1]):ULocale.getDefault() /* server default! */;
    switch(what) {
    case none:
    	response.sendRedirect(request.getContextPath()+"explore.html");
    	return;
    	//break; /*NOTREACHED*/
    	
    case locales:
    	JSONObject locales = new JSONObject();
    	LocaleDisplayNames ldn = LocaleDisplayNames.getInstance(dloc, DialectHandling.DIALECT_NAMES);
    	for(ULocale aloc : ULocale.getAvailableLocales()) {
    		locales.put(aloc.toLanguageTag(), aloc.getDisplayName(dloc));
    	}
    	o.put("locales",locales);
    	break;
    	
    case verinfo:
    	JSONObject vers = new JSONObject();
    	vers.put("icu", VersionInfo.ICU_VERSION.toString());
    	vers.put("cldr", (String)LocaleData.getCLDRVersion().toString());
    	vers.put("unicode", UCharacter.getUnicodeVersion().toString());
    	vers.put("tz", TimeZone.getTZDataVersion());
    	o.put("version", vers);
    	o.put("txt", "Powered by " + 
    		ListFormatter.getInstance(dloc).format("ICU version " + vers.get("icu"),
    											"CLDR "  + vers.get("cldr"),
    											"Unicode " + vers.get("unicode"),
    											"tz " + vers.get("tz")));
    	
    	break;
    	
    case acceptlang:
    	o.put("acceptlang", request.getHeader("Accept-Language"));
    }
	o.writeJSONString(out);
%>