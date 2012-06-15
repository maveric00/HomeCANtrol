 // Lies die Konfiguration ein

var Bilder = new Array () ;

function ReadConfig () {
  var url = "Server/NodeConf/Config.xml"; 
  var parser = new marknote.Parser();  
  
  // optional HTTP request parameters 
  // (used to construct the HTTP query string, if any) 
  
  var urlParams = {     
    param1: "zzz",     
    param2: "abc" 
  };  
  
  // parse the file 
  // (for POST requests, use "POST" instead of "GET") 
  
  var doc = parser.parseURL(url, urlParams, "GET");

  return doc ;  
}

function FindNode (Root,Name)
{
  var NameElements = Name.split("/");
  var parent=Root ;
  var child ;

  for (i=0;i<NameElements.length;i++) {
    var children=parent.getChildElements();
    for (var j=0;j<children.length;j++) {
      child = children[j] ;
      if (child.getAttributeValue("name")==NameElements[i]) break;
    } ;
    if (j==children.length) {
      child = Root ;
      break ;
    } ;
    parent = child ;
  } ;

  return child ;
}

function FindAllElements (Root,ElementType,Liste)
{
  var Children = Root.getChildElements() ;

  for (var i=0;i<Children.length;i++) {
    var child = Children[i]; 
    var Len = Liste.length ;
    
    if (child.getName()==ElementType) {
      Liste[Len]  = child ; 
    } ;

    FindAllElements(child,ElementType,Liste) ;
  } ;
}

var Bilder = new Array () ;
var BilderObjects = new Array();
var Buttons = new Array () ;
var ButtonDivs = new Array() ;

function LoadPictures (Root)
{
  FindAllElements(Root,"Bild",BilderObjects) ;

  for (var i=0;i<BilderObjects.length;i++) {
    Bilder[i] = new Image() ;
    Bilder[i].src = BilderObjects[i].getAttribute("src").getValue();
  } ;
}

function SetValue (Root,Objekt,Wert) 
{
  var Element = FindNode(Root,Objekt) ;
  if (!Element) { 
    Element.setAttribute("wert",Wert) ;
  } ;
}

function GetValue (Root,Objekt)
{
  var Element = FindNode(Root,Objekt) ;
  var Attrib = Element.getAttribute("wert") ;
  if (Attrib) {
    return (Attrib.getValue()) ;
  } else {
    return (0) ;
  } ;
}


function SetPicture (Root,Button)
{
  var i ;
  var MyBilder = Button.getChildElements("Bild") ;
  var myDiv ;

  for (i=0;i<Buttons.length;i++) if (Buttons[i]==Button) break ;
  if (i==Buttons.length) return ;// es existiert noch kein div für den Button 
  myDiv = ButtonDivs[i] ;

  // zu setzendes Bild finden
  for (i=0;i<MyBilder.length;i++) {
    var Bild = MyBilder[i] ;
    var Attr = Bild.getAttribute("objekt") ;
    var Attr2 = Bild.getAttribute("wert") ;
    if (!Attr) break ;
    if (!Attr2) break ;
    if (GetValue(Root,Attr.getValue())==Attr2.getValue()) break ;
  } ;

  if (i==MyBilder.length) return; // kein passendes Bild gefunden

  for (i=0;i<BilderObjects.length;i++) if (BilderObjects[i]==Bild) break ;

  if (i==MyBilder.length) return; // kein passendes Bild gefunden
  
  var myPic = myDiv.getElementsByTagName("img")[0] ;
  myPic.src = Bilder[i].src ;
}
  
function SetButtonSize(Button)
{
  if (!window.innerHeight) {
    var XSize = document.body.offsetWidth ;
    var YSize = document.body.offsetHeight ;
  } else {
    var XSize = window.innerWidth ;
    var YSize = window.innerHeight ;
  } ;
  var X,Y,H,B ;
  var myDiv ;

  for (i=0;i<Buttons.length;i++) if (Buttons[i]==Button) break ;
  if (i==Buttons.length) return ;// es existiert noch kein div f¡ür den Button 
  myDiv = ButtonDivs[i] ;
  
  var Position=Button.getChildElements("Position") ;
  if (!Position) return ;
  
  var AttribX = Position[0].getAttribute("x") ;
  var AttribY = Position[0].getAttribute("y") ;
  var AttribH = Position[0].getAttribute("h") ;
  var AttribB = Position[0].getAttribute("b") ;

  var myPic = myDiv.getElementsByTagName("img")[0] ;

  if (AttribX) {
    X = AttribX.getValue()*XSize/100 ;
  } else {
    alert ("X-Wert nicht gesetzt in Knopf:\n"+Button.toString()) ;
    X = 0 ;
  } ;
  if (AttribY) {
    Y = AttribY.getValue()*YSize/100 ;
  } else {
    alert ("Y-Wert nicht gesetzt in Knopf:\n"+Button.toString()) ;
    Y = 0 ;
  } ;
  if (AttribH) {
    H = AttribH.getValue()*YSize/100 ;
  } else {
    alert ("H-Wert nicht gesetzt in Knopf:\n"+Button.toString()) ;
    H = 0 ;
  } ;
  if (AttribB) {
    B = AttribB.getValue()*XSize/100 ;
  } else {
    alert ("B-Wert nicht gesetzt in Knopf:\n"+Button.toString()) ;
    B = 0 ;
  } ;

  myDiv.style.top = Y ;
  myDiv.style.left = X ;
  myPic.style.width = B ;
  myPic.style.height = H ;
}


function CreateButton (Root,Button,Page)
{
  var mydiv = document.createElement("div") ;
  mydiv.style.position="absolute";
  mydiv.style.zIndex=100 ;
  mydiv.style.margin = 0 ;
  mydiv.style.padding = 0 ;

  var myImage = document.createElement("img") ;
  myImage.src = Bilder[0].src ;
  mydiv.appendChild (myImage); 
  Page.appendChild(mydiv) ;
  Buttons[Buttons.length] = Button ;
  ButtonDivs[ButtonDivs.length] = mydiv ;
  myImage.onclick = CallAction ;

  SetButtonSize(Button) ;
  SetPicture (Root,Button);   
}

function CreatePage (Root,Webpage)
{
  var Background = Webpage.getChildElements("Bild") ;
  var Knoepfe = Webpage.getChildElements("Knopf") ;
  var i ;

  i = Knoepfe.length ;

  if (!window.innerHeight) {
    var XSize = document.body.offsetWidth ;
    var YSize = document.body.offsetHeight ;
  } else {
    var XSize = window.innerWidth ;
    var YSize = window.innerHeight ;
  } ;


  var Attr = Webpage.getAttribute("name") ;
  if (!Attr) {
    alert ("Webpage ohne Namen:\n"+Webpage.toString()) ;
    return ;
  } ;

  var mydiv = document.createElement("div") ;
  mydiv.id = Attr.getValue() ;
  mydiv.style.position="absolute" ;
  mydiv.style.zIndex=10 ;
  mydiv.style.visibility = "hidden" ;

  var myImage = document.createElement("img") ;
 
  for (i=0;i<BilderObjects.length;i++) {
    if (BilderObjects[i]==Background[0]) break ;
  } ;
  if (i==BilderObjects.length) {
    alert ("Hintergrundbild nicht gefunden\n"+Background) ;
    return ;
  } ;
  mydiv.appendChild (myImage); 
  myImage.src = Bilder[i].src ;

  mydiv.style.top = 0 ;
  mydiv.style.left = 0 ;
  mydiv.style.margin = 0 ;
  mydiv.style.padding = 0 ;
  myImage.style.width = XSize-1 ;
  myImage.style.height = YSize-1 ;

  document.body.appendChild(mydiv) ;

  for (var i=0;i<Knoepfe.length;i++) CreateButton(Root,Knoepfe[i],mydiv) ;
}
 
function CreatePages (Root) 
{
  var Web = Root.getChildElements("Web") ;
  if (!Web) {
    alert ("Keine Web-Definition in Konfiguration") ;
    return  ;
  } ;
  var Pages = Web[0].getChildElements("Seite") ;
  if (!Pages) {
    alert ("Keine Seiten-Definition in Konfiguration") ;
    return  ;
  } ;
  for (var i = 0 ; i<Pages.length;i++) CreatePage(Root,Pages[i]) ;
}

function DisplayPage (Pagename)
{
  var divs = document.body.childNodes ;

  for (var i=0;i<divs.length;i++) {
    if (divs[i].tagName) 
      if (divs[i].tagName=="DIV")
	divs[i].style.visibility = "hidden" ;
  };

  var mydiv = document.getElementById(Pagename) ;
  mydiv.style.visibility = "visible"; 
}

function ResizePage()
{
  var divs = document.body.childNodes ;
  if (!window.innerHeight) {
    var XSize = document.body.offsetWidth ;
    var YSize = document.body.offsetHeight ;
  } else {
    var XSize = window.innerWidth ;
    var YSize = window.innerHeight ;
  } ;  

  for (var i=0;i<divs.length;i++) {
    if (divs[i].tagName) 
     if (divs[i].tagName=="DIV") {
       var Childs=divs[i].childNodes ;
       for (var j=0;j<Childs.length;j++) if (Childs[j].tagName=="IMG") break ;
       if (j==Childs.length) continue ; // no Images ;
       var Image = Childs[j] ;
       Image.style.width = XSize-1 ;
       Image.style.height = YSize-1 ;
     } ;
  } ;
  for (var i=0;i<Buttons.length;i++) SetButtonSize(Buttons[i]);
}

var doc = ReadConfig() ;
var Root = doc.getRootElement() ;
LoadPictures(Root) ;

var BrowserDetect = {
  init: function () {
    this.browser = this.searchString(this.dataBrowser) || "An unknown browser";
    this.version = this.searchVersion(navigator.userAgent)
    || this.searchVersion(navigator.appVersion)
    || "an unknown version";
    this.OS = this.searchString(this.dataOS) || "an unknown OS";
  },
  searchString: function (data) {
    for (var i=0;i<data.length;i++)	{
      var dataString = data[i].string;
      var dataProp = data[i].prop;
      this.versionSearchString = data[i].versionSearch || data[i].identity;
      if (dataString) {
	if (dataString.indexOf(data[i].subString) != -1)
	  return data[i].identity;
      }
      else if (dataProp)
	return data[i].identity;
    }
  },
  searchVersion: function (dataString) {
    var index = dataString.indexOf(this.versionSearchString);
    if (index == -1) return;
    return parseFloat(dataString.substring(index+this.versionSearchString.length+1));
  },
  dataBrowser: [
  {
    string: navigator.userAgent,
    subString: "Chrome",
    identity: "Chrome"
  },
  { 	string: navigator.userAgent,
	subString: "OmniWeb",
	versionSearch: "OmniWeb/",
	identity: "OmniWeb"
  },
  {
    string: navigator.vendor,
    subString: "Apple",
    identity: "Safari",
    versionSearch: "Version"
  },
  {
    prop: window.opera,
    identity: "Opera",
    versionSearch: "Version"
  },
  {
    string: navigator.vendor,
    subString: "iCab",
    identity: "iCab"
  },
  {
    string: navigator.vendor,
    subString: "KDE",
    identity: "Konqueror"
  },
  {
    string: navigator.userAgent,
    subString: "Firefox",
    identity: "Firefox"
  },
  {
    string: navigator.vendor,
    subString: "Camino",
    identity: "Camino"
  },
  {		// for newer Netscapes (6+)
    string: navigator.userAgent,
    subString: "Netscape",
    identity: "Netscape"
  },
  {
    string: navigator.userAgent,
    subString: "MSIE",
    identity: "Explorer",
    versionSearch: "MSIE"
  },
  {
    string: navigator.userAgent,
    subString: "Gecko",
    identity: "Mozilla",
    versionSearch: "rv"
  },
  { 		// for older Netscapes (4-)
    string: navigator.userAgent,
    subString: "Mozilla",
    identity: "Netscape",
    versionSearch: "Mozilla"
  }
  ],
  dataOS : [
  {
    string: navigator.platform,
    subString: "Win",
    identity: "Windows"
  },
  {
    string: navigator.platform,
    subString: "Mac",
    identity: "Mac"
  },
  {
    string: navigator.userAgent,
    subString: "iPhone",
    identity: "iPhone/iPod"
  },
  {
    string: navigator.platform,
    subString: "Linux",
    identity: "Linux"
  }
  ]
  
};
BrowserDetect.init();

var pos = 0;

function get_appropriate_ws_url()
{
  var pcol;
  var u = document.URL;
  
  /*
   * We open the websocket encrypted if this page came on an
   * https:// url itself, otherwise unencrypted
   */
  
  if (u.substring(0, 5) == "https") {
    pcol = "wss://";
    u = u.substr(8);
  } else {
    pcol = "ws://";
    if (u.substring(0, 4) == "http")
      u = u.substr(7);
  }
  
  u = u.split('/');
  
  return pcol + u[0] + ":13248";
  alert (pcol+u[0]+":13248") ;

}


var socket_di;

if (BrowserDetect.browser == "Firefox" && BrowserDetect.version < 12) {
  socket_di = new MozWebSocket(get_appropriate_ws_url(),
			       "config-protocol");
 } else {
  socket_di = new WebSocket(get_appropriate_ws_url(),
			    "config-protocol");
 } 


try {
  socket_di.onopen = function() {
    //    document.getElementById("wsdi_statustd").style.backgroundColor = "#40ff40";
    //    document.getElementById("wsdi_status").textContent = " websocket connection opened ";
  } 
  
  socket_di.onmessage =function got_packet(msg) {
    //   document.getElementById("number").textContent = msg.data + "\n";
  } 
  
  socket_di.onclose = function(){
    //    document.getElementById("wsdi_statustd").style.backgroundColor = "#ff4040";
    // document.getElementById("wsdi_status").textContent = " websocket connection CLOSED ";
  }
 } catch(exception) {
  alert('<p>Error' + exception);  
 }

function CallAction ()
{
  var mydiv = this.parentNode ;

  for (var i = 0 ; i<ButtonDivs.length;i++) if (ButtonDivs[i]==mydiv) break ;

  if (i==ButtonDivs.length) {
    alert ("Callback-Div nicht gefunden") ;
    return ;
  } ;

  var Button = Buttons[i] ;
  var Aktion = Button.getChildElements("Aktion") ;
  if (!Aktion) {
    alert ("Knopf ohne Kommando:\n"+Button) ;
    return ;
  } ;

  var Att1 = Aktion[0].getAttribute("kommando") ;
  if (!Att1) {
    alert ("Knopf ohne Kommando:\n"+Button) ;
    return ;
  } ;

  var Att2 = Aktion[0].getAttribute("objekt") ;
  if (!Att2) {
    alert ("Knopf ohne Kommando:\n"+Button) ;
    return ;
  } ;

  var Kommando=Att1.getValue() ;
  if (Kommando=="Seite") {
    DisplayPage(Att2.getValue()) ;
  };  
  if (Kommando=="Rufe") {
    socket_di.send("Aktion " + Att2.getValue()) ;
  } ;
}


