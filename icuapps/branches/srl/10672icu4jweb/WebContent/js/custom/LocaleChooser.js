//custom.LocaleChooser
define(["dojo/_base/declare",
        "dijit/_WidgetBase", 
        "dijit/_TemplatedMixin", 
        "dojo/text!./LocaleChooser/templates/LocaleChooser.html",
        "dojo/query"],
		function(declare, WidgetBase, TemplatedMixin, template, query){
	return declare([WidgetBase, TemplatedMixin], {
		templateString: template,

		baseClass: "localeChooser",

        postCreate: function(){
            var domNode = this.domNode;
        	console.log(domNode.innerHTML);
            this.inherited(arguments);
            
            var localeListDiv =query(this.localeList);
            
            var tmp = "<ol>";
            for(var loc in Object.keys(this.locales)) {
            	tmp = tmp + "<li>"+loc+"</li>";
            }
            tmp = tmp + " </ol>";
            localeListDiv.innerHTML = tmp;
            console.log(tmp);
            // dom.create..
            
        }
		
	});
});