function mylo (fnam,lnam,dom1,dom2){
    if ( lnam == "" ) {
	var name = fnam;
    } else {
	var name = fnam + "." + lnam;
    }
    var host = dom1 + "." + dom2;
    var complete = name + "@" + host; 
    output = "<a href=" + "mail" + "to" + ":" + complete + ">" + complete + "</a>";
    document.write(output);
    return output;
}
