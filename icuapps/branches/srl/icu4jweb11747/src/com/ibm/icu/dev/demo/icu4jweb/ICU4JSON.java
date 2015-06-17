package com.ibm.icu.dev.demo.icu4jweb;

import java.io.IOException;
import java.text.ParsePosition;
import java.util.ArrayDeque;
import java.util.Arrays;
import java.util.Collection;
import java.util.Collections;
import java.util.Date;
import java.util.Deque;
import java.util.LinkedList;
import java.util.List;
import java.util.Locale;
import java.util.Map;

import javax.json.Json;
import javax.json.JsonArrayBuilder;
import javax.json.JsonObject;
import javax.json.JsonObjectBuilder;
import javax.json.JsonReader;
import javax.json.JsonStructure;
import javax.servlet.ServletException;
import javax.servlet.annotation.WebServlet;
import javax.servlet.http.HttpServlet;
import javax.servlet.http.HttpServletRequest;
import javax.servlet.http.HttpServletResponse;

import com.ibm.icu.text.DateFormat;
import com.ibm.icu.text.DateTimePatternGenerator;
import com.ibm.icu.text.SimpleDateFormat;
import com.ibm.icu.util.Calendar;
import com.ibm.icu.util.TimeZone;
import com.ibm.icu.util.ULocale;
import com.ibm.icu.util.VersionInfo;

/**
 * Servlet implementation class ICU4JSON
 */
public class ICU4JSON extends HttpServlet {
	private static final long serialVersionUID = 1L;
       
    /**
     * @see HttpServlet#HttpServlet()
     */
    public ICU4JSON() {
        super();
    }

	/**
	 * @see HttpServlet#doGet(HttpServletRequest request, HttpServletResponse response)
	 */
	protected void doPost(HttpServletRequest request, HttpServletResponse response) throws ServletException, IOException {
		request.setCharacterEncoding("utf-8");
		JsonReader reader = Json.createReader(request.getReader());
		JsonStructure json = reader.read();
		
		process(request, response, json);
	}

	protected void doGet(HttpServletRequest request, HttpServletResponse response) throws ServletException, IOException {
		process(request, response, null);
	}

	private void process(HttpServletRequest request,
			HttpServletResponse response, JsonStructure json)
			throws IOException {
		JsonObjectBuilder r = Json.createObjectBuilder().add("ICU_VERSION", VersionInfo.ICU_VERSION.toString());
		try {
			final String pi = request.getPathInfo();
			if(pi==null) return;
			final List<String> path = new LinkedList<String>(Arrays.asList(pi.split("/")));
			if(path.size()==0) return;
			path.remove(0);
			process(r, path, json, request.getParameterMap());
		} catch (Throwable t) {
			r.add("err", t.toString());
			JsonArrayBuilder jab = Json.createArrayBuilder();
			for(StackTraceElement se : t.getStackTrace()) {
				jab.add(se.toString());
			}
			r.add("err_stack", jab.build());
		} finally {
			response.setCharacterEncoding("utf-8");
			response.setContentType("application/json");
			response.getWriter().write(r.build().toString());
		}
	}
	
	private String get(Map<String,String[]>map, String key, String def) {
		if(map.containsKey(key)) {
			final String ret =  map.get(key)[0];
			if(ret.isEmpty()) return def;
			return ret;
		} else {
			return def;
		}
	}

	private void process(JsonObjectBuilder r, List<String> path, JsonStructure json, Map<String, String[]> params) {
		switch (path.get(0)) {
		case "nextRecurringEvent":
			final String tzstr = get(params, "meeting_tz", "America/Los_Angeles");
			final String dtzstr = get(params, "display_tz", tzstr);
			final String every = get(params, "every", "Wednesday");
			final String from = get(params, "from", "09:45 AM");
			final String to =  get(params, "to", "11:00 AM");
			final String locid = get(params, "loc", "en-us");
			final Locale eloc = Locale.forLanguageTag("en-us");
			final Locale loc = Locale.forLanguageTag(locid);
			final ULocale uloc = ULocale.forLocale(loc); // HACK
			final TimeZone tz = TimeZone.getTimeZone(tzstr);
			r.add("meeting_tz", tz.getID());
			final TimeZone dtz = TimeZone.getTimeZone(dtzstr);
			r.add("display_tz", dtz.getID());
			r.add("every", every);
			r.add("from", from);
			r.add("to", to);
			r.add("loc",loc.toLanguageTag());
			
			final DateTimePatternGenerator gen = DateTimePatternGenerator.getInstance(uloc);
			final SimpleDateFormat enDowFmt = new SimpleDateFormat(gen.getBestPattern("EEEE"), eloc);
			final SimpleDateFormat enTimeFmt = new SimpleDateFormat(gen.getBestPattern("hhmma"), eloc);
			final SimpleDateFormat dayFmt = new SimpleDateFormat(gen.getBestPattern("eeeedMMMy"), uloc);
			final SimpleDateFormat timeFmt = new SimpleDateFormat(gen.getBestPattern("Hmv"), uloc);
			Calendar c = Calendar.getInstance(tz);
			long now = c.getTimeInMillis();
			{
				Calendar j = parseOrThrow(enDowFmt, every, tz);
				c.set(Calendar.DAY_OF_WEEK, j.get(Calendar.DAY_OF_WEEK));
			}
			// clear seconds and ms 
			c.clear(Calendar.SECOND);
			c.clear(Calendar.MILLISECOND);
			
			// get 'to' time
			parseOrThrow(enTimeFmt, to, c);
			// did we end up in the past? if so, roll forward.
			if(c.getTimeInMillis() < now) {
				// need the next week
				c.add(Calendar.WEEK_OF_YEAR, 1);
				r.add("nextWeek", true);
			}
			long toTime = c.getTimeInMillis();
			// get 'from' time
			parseOrThrow(enTimeFmt, from, c);
			if(toTime < c.getTimeInMillis()) {
				// need the next day
				c.add(Calendar.DAY_OF_MONTH, 1);
				r.add("endsTomorrow", true);
			}
			long fromTime = c.getTimeInMillis();
			
			if((toTime >= now) && (fromTime <= now)) {
				r.add("onNow", true); // going on now!
			}
			
			// for now, use 'core' tz
			timeFmt.setTimeZone(dtz);
			dayFmt.setTimeZone(dtz);
			
			r.add("now",now);
			r.add("nowTimeStr", timeFmt.format(now));
			r.add("nowDayStr", dayFmt.format(now));
			r.add("toTimeStr", timeFmt.format(toTime));
			r.add("toTime",toTime);
			r.add("toDayStr", dayFmt.format(toTime));
			r.add("fromTime", fromTime);
			r.add("fromTimeStr", timeFmt.format(fromTime));
			r.add("fromDayStr", dayFmt.format(fromTime));
		}
	}

	private Calendar parseOrThrow(SimpleDateFormat fmt, String str,
			TimeZone tz) {
		Calendar j;
		j = Calendar.getInstance(tz);
		parseOrThrow(fmt, str, j);
		return j;
	}

	private Calendar parseOrThrow(final SimpleDateFormat fmt, final String str) {
		Calendar j;
		j = Calendar.getInstance();
		parseOrThrow(fmt, str, j);
		return j;
	}

	private void parseOrThrow(final SimpleDateFormat fmt, final String str,
			Calendar cal) {
		ParsePosition pp = new ParsePosition(0);
		fmt.parse(str, cal, pp);
		if(pp.getErrorIndex()!=-1 || pp.getIndex()!=str.length()) {
			throw new IllegalArgumentException("Failed to parse '"+str+"' through pattern " + fmt.toPattern() + " - try a string like '" + fmt.format(new Date())+"'");
		}
	}
}
