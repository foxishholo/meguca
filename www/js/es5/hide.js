"use strict";System.register([],function(e,r){var t,n;return{setters:[],execute:function(){t=require("./main"),n=new t.Memory("hide",7,!0),t.reply("hide",function(e){if(!e.get("mine")){var r=n.write(e.get("num"));e.remove(),t.request("hide:render",r)}}),t.reply("hide:clear",function(){return n.purgeAll()}),t.defer(function(){return t.request("hide:render",n.size())})}}});
//# sourceMappingURL=maps/hide.js.map