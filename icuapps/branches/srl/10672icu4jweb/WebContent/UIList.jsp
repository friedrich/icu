<%@page import="com.ibm.icu.text.LocaleDisplayNames.DialectHandling"%>
<%@page import="java.util.Collections"%>
<%@page import="java.util.Set"%>
<%@page import="java.util.List"%>
<%@page import="com.ibm.icu.util.*"%>
<%@page import="java.util.TreeSet"%>
<%@page   import="com.ibm.icu.text.*"  %>
<%@ page language="java" contentType="text/html; charset=UTF-8"
    pageEncoding="UTF-8"
    %>
<!DOCTYPE html PUBLIC "-//W3C//DTD HTML 4.01 Transitional//EN" "http://www.w3.org/TR/html4/loose.dtd">
<html>
<head>
<meta http-equiv="Content-Type" content="text/html; charset=UTF-8">
<title>Insert title here</title>
</head>
<body>

<%
Set<ULocale> s = new TreeSet<ULocale>();
for(ULocale l : com.ibm.icu.util.ULocale.getAvailableLocales()) {
	//s.add(l);
}
s.add(ULocale.forLanguageTag("en"));
s.add(ULocale.forLanguageTag("de"));
s.add(ULocale.forLanguageTag("ta"));
s.add(ULocale.forLanguageTag("chr"));
 List<LocaleDisplayNames.UiListItem> list = LocaleDisplayNames.getInstance(ULocale.forLanguageTag("en-AU"),DialectHandling.DIALECT_NAMES)
		 	.getUiList(s, true, Collator.getInstance());


%>

<ul>
<% for(LocaleDisplayNames.UiListItem l : list) { %>
	<li> <%= l.nameInSelf %></li>
<% } %>
</ul>

</body>
</html>