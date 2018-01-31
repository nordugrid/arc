function ngurl(link)
{
    var wloc="http://"+document.domain+link;
    var vtest=link;
    var prot=vtest.substring(0,4);
    var vhttp="http";
    if (prot == vhttp) {
      var wloc=link
    }
    return wloc;
}

function monitor(link,x,y,n) 
{
    // "n" is needed to keep dedicated windows for each monitor type
    // function ngurl() adds HTTP contact string, if needed
    //  wloc=ngurl(link);
    var ua           = ' ' + navigator.userAgent.toLowerCase();
    var is_opera     = ua.indexOf('opera');
    var is_lynx      = ua.indexOf('lynx');
    var is_konqueror = ua.indexOf('konqueror');
    wloc = link;
    browser = navigator.appName;
    if ( is_opera>0 || is_lynx>0 || is_konqueror>0 ) {
	window.location = wloc;
    } else {
	aaa=open("","win"+n,"innerWidth="+x+",innerHeight="+y+",resizable=1,scrollbars=1,width="+x+",height="+y);
	aaa.document.encoding = "text/html; charset=utf-8";
	aaa.document.clear();
	aaa.document.writeln("<!DOCTYPE HTML PUBLIC '-//W3C//DTD W3 HTML//EN'>");
	aaa.document.writeln("<html>");
	aaa.document.writeln("<head><title>NorduGrid</title>");
	aaa.document.writeln("<META HTTP-EQUIV='CONTENT-TYPE' CONTENT='text/html; charset=utf-8'>");
	aaa.document.writeln("</head>");
	aaa.document.writeln("<body bgcolor=#ffffff text=#990000>");
	aaa.document.writeln("<p align=center style={font-size:large;color:990000;font-family:sans-serif;font-style:italic;text-decoration:blink;}><br><br><br><br><br>");
	aaa.document.writeln("Collecting information...");
	aaa.document.writeln("</p>");
	aaa.document.writeln("</body>");
	aaa.document.writeln("</html>");
	aaa.document.close();
	aaa.document.location.href=wloc;
	aaa.document.close();
    }
}

