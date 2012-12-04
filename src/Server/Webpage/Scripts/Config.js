 // Lies die Konfiguration ein

var Sprache = 0 ;
var Bilder = new Array () ;

function ReadConfig () {
  var url = "NodeConf/Config.xml"; 
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

var BilderObjects = new Array();
var Dimmer = new Array();
var Buttons = new Array () ;
var ButtonDivs = new Array() ;

function LoadPictures (Root)
{
  if (Sprache==0) {
    FindAllElements(Root,"Bild",BilderObjects) ;
  } else {
    FindAllElements(Root,"Picture",BilderObjects) ;
  } ;

  for (var i=0;i<BilderObjects.length;i++) {
    Bilder[i] = new Image() ;
    Bilder[i].src = BilderObjects[i].getAttribute("src").getValue();
  } ;
  Dimmer[0] = new Image() ;
  Dimmer[0].src = FindNode(Root,"Webpage/MarkX").getAttribute("src").getValue() ;
  Dimmer[1] = new Image() ;
  Dimmer[1].src = FindNode(Root,"Webpage/MarkY").getAttribute("src").getValue() ;
  Dimmer[2] = new Image() ;
  Dimmer[2].src = FindNode(Root,"Webpage/MarkXY").getAttribute("src").getValue() ;
}


function GetValue (Root,Objekt)
{
  var Element = FindNode(Root,Objekt) ;
  var Attrib ;
  if (Sprache==0) {
    Attrib = Element.getAttribute("wert") ;
  } else {
    Attrib = Element.getAttribute("value") ;
  } ;
  if (Attrib) {
    return (Attrib.getValue()) ;
  } else {
    return (0) ;
  } ;
}


function SetPicture (Root,Button)
{
  var i ;
  var MyBilder ;
  var myDiv ;
  var MyActions ;

  if (Sprache==0) {
    MyBilder = Button.getChildElements("Bild") ;
  } else {
    MyBilder = Button.getChildElements("Picture") ;
  } ;

  if (!MyBilder) {
    // button enthaelt kein Bild sondern html??
    return ;
  }
  for (i=0;i<Buttons.length;i++) if (Buttons[i]==Button) break ;
  if (i==Buttons.length) return ;// es existiert noch kein div fuer den Button 
  myDiv = ButtonDivs[i] ;

  // zu setzendes Bild finden
  for (i=0;i<MyBilder.length;i++) {
    var Bild = MyBilder[i] ;
    var Attr ;
    var Attr2 ;
    if (Sprache==0) {
      Attr = Bild.getAttribute("objekt") ;
      Attr2 = Bild.getAttribute("wert") ;
    } else {
      Attr = Bild.getAttribute("object") ;
      Attr2 = Bild.getAttribute("value") ;
    } ;
    if (!Attr) break ;
    if (!Attr2) break ;
    if (GetValue(Root,Attr.getValue())==Attr2.getValue()) break ;
  } ;

  if (i==MyBilder.length) return; // kein passendes Bild gefunden

  for (i=0;i<BilderObjects.length;i++) if (BilderObjects[i]==Bild) break ;

  if (i==BilderObjects.length) return; // kein passendes Bild gefunden
  
  var myPic = myDiv.getElementsByTagName("img")[0] ;
  myPic.src = Bilder[i].src ;



  // Nun noch ggf den Dimmer-Marker setzen

  var markX = 0 ;
  var xVal=0 ;
  var markY = 0 ;
  var yVal=0 ;

  if (Sprache==0) {
    MyActions = Button.getChildElements("Aktion") ;
  } else {
    MyActions = Button.getChildElements("Action") ;
  } ;
  
  for (i=0;i<MyActions.length;i++) {
    if (Sprache==0) {
      Attr = MyActions[i].getAttribute("kommando") ;
      Attr2 = MyActions[i].getAttribute("objekt") ;
    } else {
      Attr = MyActions[i].getAttribute("command") ;      
      Attr2 = MyActions[i].getAttribute("object") ;
    } ;
    if (Attr) {
      if ((Attr.getValue()=="Dim-X")||(Attr.getValue()=="Dimme-X")) {
	if (Attr2) {
	  markX=1 ;
	  xVal = GetValue(Root,Attr2.getValue()); 
	} ;
      } ;
      if ((Attr.getValue()=="Dim-Y")||(Attr.getValue()=="Dimme-Y")) {
	if (Attr2) {
	  markY=1 ;
	  yVal = GetValue(Root,Attr2.getValue()); 
	} ;
      } ;
      if ((Attr.getValue()=="Dim-IY")||(Attr.getValue()=="Dimme-IY")) {
	if (Attr2) {
	  markY=-1 ;
	  yVal = GetValue(Root,Attr2.getValue()); 
	} ;
      } ;
    } ;
  } ;

  // hier ist bekannt, ob X- oder Y-Marker gesetzt werden sollen, und bei welchem Prozentwert diese gesetzt werden sollen...

  var myDim = myDiv.getElementsByTagName("div")[0] ;    

  if (markX==1) {
    myDim.style.left = (xVal*myPic.offsetWidth/100) ;
  } ;
  if (markY==1) {
    myDim.style.top = (yVal*myPic.offsetHeight/100) ;
  } ;
  if (markY==-1) {
    myDim.style.top = myPic.offsetHeight-(yVal*myPic.offsetHeight/100) ;
  } ;
}

function SetValue (Root,Objekt,Wert) 
{
  var Element = FindNode(Root,Objekt) ;
  if (!Element) { 
    if (Sprache==0) {
      Element.setAttribute("wert",Wert) ;
    } else {
      Element.setAttribute("value",Wert) ;
    } ;
    SetPicture(Root,Element) ;
  } ;
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
  var myDim = myDiv.getElementsByTagName("img")[1] ;
  var myiframe = myDiv.getElementsByTagName("iframe")[0] ;

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
  myDiv.style.minWidth = B ;
  myDiv.style.maxWidth = B ;
  myDiv.style.minHeight = H ;
  myDiv.style.maxHeight = H ;
  myDiv.style.width = B ;
  myDiv.style.height = H ;
  if (myPic) {
    myPic.style.width = B ;
    myPic.style.height = H ;
  } ;
  if (myDim) {
    if (myDim.src==Dimmer[0].src) {
      // X-Dimmer, nur hoehe anpassen
      myDim.style.height = H ;
    } else if (myDim.src==Dimmer[1].src) {
      myDim.style.width = B ;
    } ;
    // XY-Dimmer wird nicht angepasst...
  } ;
  if (myiframe) {
    myiframe.style.width = B ;
    myiframe.style.height = H ;
  } ;
  // Nun noch ggf den Dimmer-Marker setzen

  var markX = 0 ;
  var xVal=0 ;
  var markY = 0 ;
  var yVal=0 ;

  if (Sprache==0) {
    MyActions = Button.getChildElements("Aktion") ;
  } else {
    MyActions = Button.getChildElements("Action") ;
  } ;
  
  for (i=0;i<MyActions.length;i++) {
    if (Sprache==0) {
      Attr = MyActions[i].getAttribute("kommando") ;
      Attr2 = MyActions[i].getAttribute("objekt") ;
    } else {
      Attr = MyActions[i].getAttribute("command") ;      
      Attr2 = MyActions[i].getAttribute("object") ;
    } ;
    if (Attr) {
      if ((Attr.getValue()=="Dim-X")||(Attr.getValue()=="Dimme-X")) {
	if (Attr2) {
	  markX=1 ;
	  xVal = GetValue(Root,Attr2.getValue()); 
	} ;
      } ;
      if ((Attr.getValue()=="Dim-Y")||(Attr.getValue()=="Dimme-Y")) {
	if (Attr2) {
	  markY=1 ;
	  yVal = GetValue(Root,Attr2.getValue()); 
	} ;
      } ;
      if ((Attr.getValue()=="Dim-IY")||(Attr.getValue()=="Dimme-IY")) {
	if (Attr2) {
	  markY=-1 ;
	  yVal = GetValue(Root,Attr2.getValue()); 
	} ;
      } ;
    } ;
  } ;

  // hier ist bekannt, ob X- oder Y-Marker gesetzt werden sollen, und bei welchem Prozentwert diese gesetzt werden sollen...
  var myDimDiv = myDiv.getElementsByTagName("div")[0] ;    


  if (markX==1) {
    myDimDiv.style.left = (xVal*B/100) ;
  } ;
  if (markY==1) {
    myDimDiv.style.top = (yVal*H/100) ;
  } ;
  if (markY==-1) {
    myDimDiv.style.top = H-(yVal*H/100) ;
  } ;

}


function CreateButton (Root,Button,Page)
{
  var mydiv = document.createElement("div") ;
  mydiv.style.position="absolute";
  mydiv.style.zIndex=100 ;
  mydiv.style.margin = 0 ;
  mydiv.style.padding = 0 ;

  var zindex = Button.getAttribute("zindex") ;
  if (zindex) {
    mydiv.style.zIndex = zindex.getValue() ;
  } ;

  var Inhalt ;
  if (Sprache==0) {
    Inhalt = Button.getChildElements("Inhalt") ;
  } else {
    Inhalt = Button.getChildElements("Content") ;
  } ;
  if (Inhalt.length>0) {    
    var html = Inhalt[0].getAttribute("html") ;
    if (html) {
      mydiv.innerHTML = html.getValue() ;
    } ;
    var overflo = Inhalt[0].getAttribute("overflow") ;
    if (html) {
      mydiv.style.overflow = overflo.getValue() ;
    } ;
  } else {
    var myImage = document.createElement("img") ;
    myImage.src = Bilder[0].src ;
    mydiv.appendChild (myImage); 
    var Aktion ;
    var Attr ;
    var Attr2 ;
    if (Sprache==0) {
      Aktion = Button.getChildElements("Aktion") ;
      Attr = Aktion[0].getAttribute("kommando") ;
    } else {
      Aktion = Button.getChildElements("Action") ;
      Attr = Aktion[0].getAttribute("command") ;
    } ;
    if (Attr) {
      if ((Attr.getValue()=="Dim-X")||(Attr.getValue()=="Dimme-X")) {
	// Einen Dimmer-Marker anfuegen
	var dimdiv = document.createElement("div") ;
	dimdiv.style.position="absolute";
	dimdiv.style.zIndex=mydiv.style.zIndex+1 ;
	dimdiv.style.margin = 0 ;
	dimdiv.style.padding = 0 ;
	dimdiv.style.top = 0 ;
	dimdiv.style.left = 0 ;
	mydiv.appendChild (dimdiv); 
	var myDimmer = document.createElement("img") ;
	myDimmer.src = Dimmer[0].src ;
	dimdiv.appendChild (myDimmer); 
      } ;
      if ((Attr.getValue()=="Dim-Y")||(Attr.getValue()=="Dimme-Y")||
	  (Attr.getValue()=="Dim-IY")||(Attr.getValue()=="Dimme-IY")) {
	var dimdiv = document.createElement("div") ;
	dimdiv.style.position="absolute";
	dimdiv.style.zIndex=mydiv.style.zIndex+1 ;
	dimdiv.style.margin = 0 ;
	dimdiv.style.padding = 0 ;
	dimdiv.style.top = 0 ;
	dimdiv.style.left = 0 ;
	mydiv.appendChild (dimdiv); 
	var myDimmer = document.createElement("img") ;
	myDimmer.src = Dimmer[1].src ;
	dimdiv.appendChild (myDimmer); 
      } ;
      if ((Attr.getValue()=="Dim-XY")||(Attr.getValue()=="Dimme-XY")) {
	var dimdiv = document.createElement("div") ;
	dimdiv.style.position="absolute";
	dimdiv.style.zIndex=mydiv.style.zIndex+1 ;
	dimdiv.style.margin = 0 ;
	dimdiv.style.padding = 0 ;
	dimdiv.style.top = 0 ;
	dimdiv.style.left = 0 ;
	mydiv.appendChild (dimdiv); 
	var myDimmer = document.createElement("img") ;
	myDimmer.src = Dimmer[2].src ;
	dimdiv.appendChild (myDimmer); 
      } ;
    }
    if (Aktion.length>0) {
      myImage.onclick = CallAction ;
    } ;
  } ;
  
  Page.appendChild(mydiv) ;
  Buttons[Buttons.length] = Button ;
  ButtonDivs[ButtonDivs.length] = mydiv ;

  SetButtonSize(Button) ;
  SetPicture (Root,Button);   
}

function CreatePage (Root,Webpage)
{
  var Background ;
  var Knoepfe ;
  if (Sprache==0) {
    Background = Webpage.getChildElements("Bild") ;
    Knoepfe = Webpage.getChildElements("Knopf") ;
  } else {
    Background = Webpage.getChildElements("Picture") ;
    Knoepfe = Webpage.getChildElements("Button") ;
  } ;
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
  var Pages ;
  if (Sprache==0) {
    Pages = Web[0].getChildElements("Seite") ;
  } else {
    Pages = Web[0].getChildElements("Page") ;
  } ;

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
var SprachObjekt = new Array();
var Root = doc.getRootElement() ;

FindAllElements(Root,"Sprache",SprachObjekt) ;
if (SprachObjekt.length==0) {
  FindAllElements(Root,"Language",SprachObjekt) ;
  if (SprachObjekt.length>0) {
    Sprache = 1 ;
  } ;
} ;

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
  var PortObjekt = new Array();
  var Root = doc.getRootElement() ;
  var Attr ;

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

  FindAllElements(Root,"Port",PortObjekt) ;
  if (PortObject[0]) {
    Attr = PortObject[0].getAttribute("WS") ;
  } ;

  if (Attr) {
    return pcol + u[0] + ":"+Attr.getValue();
    alert (pcol+u[0]+":"+Attr.getValue()) ;
  } else {
    return pcol + u[0] + ":13248";
    alert (pcol+u[0]+":13248") ;
  } ;

}


var socket_di;

try {
if (BrowserDetect.browser == "Firefox" && BrowserDetect.version < 12) {
  socket_di = new MozWebSocket(get_appropriate_ws_url(),
			       "config-protocol");
 } else {
  socket_di = new WebSocket(get_appropriate_ws_url(),
			    "config-protocol");
 } 
} catch(exception) {
  alert ("Browser does not support Websockets!") ;
 }

try {
  socket_di.onopen = function() {
    //    document.getElementById("wsdi_statustd").style.backgroundColor = "#40ff40";
    //    document.getElementById("wsdi_status").textContent = " websocket connection opened ";
  } 
  
  socket_di.onmessage =function got_packet(msg) {
    var CommandElements = msg.data.split(" ") ;
    if (CommandElements[0]=="Set") {
      // Setze Wert auf den angegebenen
      SetValue(Root,CommandElements[1],parseInt(CommandElements[2])) ;
    } ;
  } 
  
  socket_di.onclose = function(){
    //    document.getElementById("wsdi_statustd").style.backgroundColor = "#ff4040";
    // document.getElementById("wsdi_status").textContent = " websocket connection CLOSED ";
  }
 } catch(exception) {
 }

function getX(el) {
  x = el.offsetLeft;
  if (!el.offsetParent) return x;
  else return (x+getX(el.offsetParent));
}

function getY (el) {
  y = el.offsetTop;
  if (!el.offsetParent) return y;
  else return (y+getY(el.offsetParent));
}




function CallAction (e)
{
  if (!e) var e = window.event;
  var x;
  var y;
  if (e.pageX || e.pageY) { 
    x = e.pageX;
    y = e.pageY;
  }
  else { 
    x = e.clientX + document.body.scrollLeft + document.documentElement.scrollLeft; 
    y = e.clientY + document.body.scrollTop + document.documentElement.scrollTop; 
  } 
  x -= getX(this) ;
  y -= getY(this) ;

   
  var mydiv = this.parentNode ;

  for (var i = 0 ; i<ButtonDivs.length;i++) if (ButtonDivs[i]==mydiv) break ;

  if (i==ButtonDivs.length) {
    alert ("Callback-Div nicht gefunden") ;
    return ;
  } ;

  var Button = Buttons[i] ;
  var Aktion ;
  if (Sprache==0) {
    Aktion = Button.getChildElements("Aktion") ;
  } else {
    Aktion = Button.getChildElements("Action") ;
  } ;
  if (!Aktion) {
    alert ("Knopf ohne Kommando:\n"+Button) ;
    return ;
  } ;

  var Attr1 ;
  if (Sprache==0) {
    Att1 = Aktion[0].getAttribute("kommando") ;
  } else {
    Att1 = Aktion[0].getAttribute("command") ;
  } ;
  if (!Att1) {
    alert ("Knopf ohne Kommando:\n"+Button) ;
    return ;
  } ;

  var Att2 ;
  if (Sprache==0) {
    Att2 = Aktion[0].getAttribute("objekt") ;
  } else {
    Att2 = Aktion[0].getAttribute("object") ;
  } ;
  if (!Att2) {
    alert ("Knopf ohne Kommando:\n"+Button) ;
    return ;
  } ;

  var Kommando=Att1.getValue() ;
  if ((Kommando=="Seite")||(Kommando=="Page")) {
    DisplayPage(Att2.getValue()) ;
  };  
  

  if ((Kommando=="Rufe")||(Kommando=="Call")) {
    try {
      socket_di.send("Aktion " + Att2.getValue()) ;
    } catch (exception) {
      alert ("Aktion " + Att2.getValue()); 
    }
  } ;

  if ((Kommando=="Dimme-X")||(Kommando=="Dim-X")) {
    try {
      socket_di.send("Dim " + Att2.getValue()+ " to "+(x*100/this.width).toPrecision(4)) ;
    } catch (exception) {
      alert("Dim X " + Att2.getValue()+ " to "+(x*100/this.width).toPrecision(4)) ;
    } ;
  } ;

  if ((Kommando=="Dimme-Y")||(Kommando=="Dim-Y")) {
    try {
      socket_di.send("Dim " + Att2.getValue()+ " to "+(y*100/this.height).toPrecision(4)) ;
    } catch (exception) {
      alert("Dim Y " + Att2.getValue()+ " to "+(y*100/this.height).toPrecision(4)) ;
    } ;
  } ;

  if ((Kommando=="Dimme-IY")||(Kommando=="Dim-IY")) {
    try {
      socket_di.send("Dim " + Att2.getValue()+ " to "+(y*100/this.height).toPrecision(4)) ;
    } catch (exception) {
      alert("Dim IY " + Att2.getValue()+ " to "+((this.height-y)*100/this.height).toPrecision(4)) ;
    } ;
  } ;

}


