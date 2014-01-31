require(["dojo/query", "dojo/request", "dojo/dom", "dojo/dom-construct", 
         "dojo/main", "dojo/fx", 
         "custom/LocaleChooser",
         "dojo/domReady!"],
 function explore(query,request,dom, dcons, main, fx, LocaleChooser) {
	function fetch(param, funcok, funcerr) {
		var url = "json/"+param.what;
		console.log("Fetch: " + url);
		request
		.get(url, {handleAs: 'json'})
		.then(function onok(json){
			console.log("OK: " + url + " - read: " + Object.keys(json));
			funcok(json);
		})
		.otherwise(function onerr(err) {
			console.log("ERR: " + url + " -> " + err);
			if (funcerr) {
				funcerr(err);
			}
		});
	}
	
	 function getIcuVersion(lang) {
		 fetch({what:"verinfo" /* lang - ignored */ },
				 function(jsonIcuVer) {
			 		console.log("ICU Version: " + JSON.stringify(jsonIcuVer));

			 	    var icuver = dom.byId("icuver");
			 	    icuver.style.top='-100px';
			 	    
			 	    icuver.innerHTML = jsonIcuVer.txt;
			 	    			 	    
			 	    
			 	    fx.slideTo({
			 	        top: 10,
			 	        left: 100,
			 	        node: icuver
			 	    }).play();
			 	    
		 	});
	 }
	 
	 
	 function showStuff() {
		 fetch({what:"acceptlang"}, function(jsonAcceptLang) {
             console.log("Accept-Language: " + jsonAcceptLang.acceptlang);
        	 getIcuVersion(jsonAcceptLang.acceptLang);
        	 fetch({what:"locales"}, function(jsonLocales) {
        		 var localeChooserDiv = dom.byId("localeChooser");
        		 var widget = new LocaleChooser({locales: jsonLocales.locales}).placeAt(localeChooserDiv);
        	 });
		 });
	 }
	 
	 showStuff();
 });