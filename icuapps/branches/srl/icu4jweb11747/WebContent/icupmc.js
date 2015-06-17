require(["dojo/query", "dojo/request", "dojo/dom", "dojo/dom-construct","dojo/domReady!"],
function icupmc(query,request,dom,dcons) {
	var theDiv = dom.byId('icupmc');
	
	theDiv.appendChild(document.createTextNode('loading...'));
	
	request
		.get('api/nextRecurringEvent?loc=en-us&display_tz='+window.location.search.substring(1), {handleAs: 'json'})
		.then(function(json) {
			if(json.err) {
				console.log(JSON.stringify(json));
				theDiv.appendChild(document.createTextNode(json.err));
			} else {
				theDiv.innerHTML="";
				
				if(json.nextWeek) {
					theDiv.appendChild(document.createTextNode('Next Week: '));
				}
				theDiv.appendChild(document.createTextNode(json.fromDayStr));
				theDiv.appendChild(document.createElement('p'));
				theDiv.appendChild(document.createTextNode('From: '));
				theDiv.appendChild(document.createTextNode(json.fromTimeStr));
				theDiv.appendChild(document.createElement('p'));
				theDiv.appendChild(document.createTextNode('To: '));
				theDiv.appendChild(document.createTextNode(json.toTimeStr));
				if(json.fromDayStr != json.toDayStr) {
					theDiv.appendChild(document.createTextNode(' '));
					theDiv.appendChild(document.createTextNode(json.toDayStr));
				}
			}
		})
		.otherwise(function(err) {
			console.log(err);
			theDiv.appendChild(document.createTextNode(err));
		});
});
