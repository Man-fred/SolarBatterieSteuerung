// synchron halten mit Arduino:
// 
const SPECIAL_MAX = 15;
const SPECIAL_NAME = 32;
const SPECIAL_VAL_MAX = 8;
const special = {
    name: '',
    val: []
};

var myAjax;
var myAjaxInterval;
var myObj;
var page2active = 0;
var ajaxOnline = false;
var ajaxError = 0;
var Url = "";
var aktuelleSeite = "";
var StatusArr;
var small = "";
var notifId = 0;
var userInit = 1;
(function (window, document) {
    var layout = document.getElementById('layout'),
        menu = document.getElementById('menu'),
        menuLink = document.getElementById('menuLink'),
        content = document.getElementById('main');
    var inhalt = '';
    for (i = 0; i < SPECIAL_MAX; i++) { //Zeilen  
        inhalt += '<div id="specialdiv' + i + '" class="clear">';
        inhalt += '<div class="left">'+(i==0?'Name ':'')+'<br><input name="name_r' + i + '" id="name_r' + i + '" value="" type="text"></div>';
        
        inhalt += '<div class="left">'+(i==0?'Typ ':'')+'<br><select name="setup' + i + '_0" id="setup' + i + '_0" size="1" onchange="typeSelect(' + i + ',this.value)" style="text-align:center;">' +
        '<option value="99">ungenutzt</option><option value="0">Regelung</option><option value="1">DTU</option><option value="2">Wechselrichter</option><option value="3">Strommeter</option><option value="4">Batterie</option></select></div>';
        
        inhalt += '<div class="left">'+(i==0?'Leitung ':'')+'<br><input name="setup' + i + '_2" id="setup' + i + '_2" value="" pattern="^[0-9]*$" size="4" type="text"></div>';
        inhalt += '<div class="right">';
        inhalt += '<div id="setup7div' + i + '" class="left hide">Gerät<br><select name="setup' + i + '_7" id="setup' + i + '_7" value="" pattern="^[0-9]$" size="1">'+
        '<option value="99">99</option><option value="0">00</option><option value="1">01</option></select></div>';
        
        inhalt += '<div id="addressdiv' + i + '" class="left hide">Adresse<br><input name="address' + i + '" id="address' + i + '" value="" type="text"></div>';
        inhalt += '<div id="devuserdiv' + i + '" class="left hide">User<br><input name="devuser' + i + '" id="devuser' + i + '" value="" type="text"></div>';
        inhalt += '<div id="devpassdiv' + i + '" class="left hide">Passwort<br><input name="devpass' + i + '" id="devpass' + i + '" value="" type="text"></div>';
        inhalt += '<div id="setup3div' + i + '" class="left hide">Gerätenr<br><input name="setup' + i + '_3" id="setup' + i + '_3" value="" pattern="^[0-9]*$" size="4" type="text"></div>';
        inhalt += '<div id="setup1div' + i + '" class="left hide">lfd.Nr<br><input name="setup' + i + '_1" id="setup' + i + '_1" value="" pattern="^[0-9]$" size="4" type="text"></div>';
        inhalt += '<div id="setup4div' + i + '" class="left hide"><span id="setup4text' + i + '">Res</span> W<br><input name="setup' + i + '_4" id="setup' + i + '_4" value="" pattern="^[0-9]*$" size="4" type="text"></div>';
        inhalt += '<div id="setup5div' + i + '" class="left hide">Min W<br><input name="setup' + i + '_5" id="setup' + i + '_5" value="" pattern="^[0-9]*$" size="4" type="text"></div>';
        inhalt += '<div id="setup6div' + i + '" class="left hide">Limit %<br><input name="setup' + i + '_6" id="setup' + i + '_6" value="" pattern="^[0-9]*$" size="4" type="text"></div>';
        inhalt += '</div></div>';
    }
    document.getElementById('special_config').innerHTML = inhalt;
    if (window.location.hash)
        aktuelleSeite = window.location.hash.substring(1);

    function toggleClass(element, className) {
        var classes = element.className.split(/\s+/),
            length = classes.length,
            i = 0;

        for (; i < length; i++) {
            if (classes[i] === className) {
                classes.splice(i, 1);
                break;
            }
        }
        // The className is not found
        if (length === classes.length) {
            classes.push(className);
        }

        element.className = classes.join(' ');
    }

    function toggleAll(e) {
        var active = 'active';

        e.preventDefault();
        toggleClass(layout, active);
        toggleClass(menu, active);
        toggleClass(menuLink, active);
    }

    menuLink.onclick = function (e) {
        toggleAll(e);
    };

    content.onclick = function (e) {
        if (menu.className.indexOf('active') !== -1) {
            toggleAll(e);
        }
    };
    menu.onclick = function (e) {
        if (menu.className.indexOf('active') !== -1) {
            toggleAll(e);
        }
    };

    myAjax = new XMLHttpRequest();
    if (!myAjax) {
        notification("error", "Kann keine XMLHTTP-Instanz erzeugen");
        console.log("myAjax Fehler");
        return false;
    } else {
        myAjax.onreadystatechange = LesenAjax;
        console.log("myAjax ok");
    }

    load_page(aktuelleSeite, true);
}(this, this.document));

function typeSelect(nr, type) {
    if (type == 0) { // Leitung
        document.getElementById('specialdiv' + nr).classList.value = 'clear liveR';
        document.getElementById('addressdiv' + nr).classList.add('hide');
        document.getElementById('devuserdiv' + nr).classList.add('hide');
        document.getElementById('devpassdiv' + nr).classList.add('hide');
        document.getElementById('setup3div' + nr).classList.add('hide');
        document.getElementById('setup1div' + nr).classList.add('hide');
        document.getElementById('setup4div' + nr).classList.remove('hide');
        document.getElementById('setup4text' + nr).innerHTML = 'Res';
        document.getElementById('setup5div' + nr).classList.remove('hide');
        document.getElementById('setup6div' + nr).classList.add('hide');
        document.getElementById('setup7div' + nr).classList.add('hide');
    }
    else if (type == 1) { // DTU
        document.getElementById('specialdiv' + nr).classList.value = 'clear liveD';
        document.getElementById('addressdiv' + nr).classList.remove('hide');
        document.getElementById('devuserdiv' + nr).classList.remove('hide');
        document.getElementById('devpassdiv' + nr).classList.remove('hide');
        document.getElementById('setup3div' + nr).classList.remove('hide');
        document.getElementById('setup1div' + nr).classList.add('hide');
        document.getElementById('setup4div' + nr).classList.add('hide');
        document.getElementById('setup5div' + nr).classList.add('hide');
        document.getElementById('setup6div' + nr).classList.add('hide');
        document.getElementById('setup7div' + nr).classList.remove('hide');
        document.getElementById('setup' + nr + '_7').innerHTML = '<option value="0">ahoyDTU</option><option value="1">openDTU</option>';

    }
    else if (type == 2) { // WR
        document.getElementById('specialdiv' + nr).classList.value = 'clear liveW';
        document.getElementById('addressdiv' + nr).classList.add('hide');
        document.getElementById('devuserdiv' + nr).classList.add('hide');
        document.getElementById('devpassdiv' + nr).classList.add('hide');
        document.getElementById('setup3div' + nr).classList.remove('hide');
        document.getElementById('setup1div' + nr).classList.remove('hide');
        document.getElementById('setup4div' + nr).classList.remove('hide');
        document.getElementById('setup4text' + nr).innerHTML = 'Max';
        document.getElementById('setup5div' + nr).classList.remove('hide');
        document.getElementById('setup6div' + nr).classList.remove('hide');
        document.getElementById('setup7div' + nr).classList.remove('hide');
        document.getElementById('setup' + nr + '_7').innerHTML = '<option value="0">Solar</option><option value="1">Batterie</option><option value="2">gemischt</option>';
    }
    else if (type == 3) { // Meter
        document.getElementById('specialdiv' + nr).classList.value = 'clear liveM';
        document.getElementById('addressdiv' + nr).classList.remove('hide');
        document.getElementById('devuserdiv' + nr).classList.remove('hide');
        document.getElementById('devpassdiv' + nr).classList.remove('hide');
        document.getElementById('setup3div' + nr).classList.remove('hide');
        document.getElementById('setup1div' + nr).classList.remove('hide');
        document.getElementById('setup4div' + nr).classList.add('hide');
        document.getElementById('setup5div' + nr).classList.add('hide');
        document.getElementById('setup6div' + nr).classList.add('hide');
        document.getElementById('setup7div' + nr).classList.remove('hide');
        document.getElementById('setup' + nr + '_7').innerHTML = '<option value="0">cfos</option><option value="1">Shelly EM3</option><option value="2">Shelly 2.5</option>';
    }
    else if (type == 4) { // Batterie
        document.getElementById('specialdiv' + nr).classList.value = 'clear liveB';
        document.getElementById('addressdiv' + nr).classList.remove('hide');
        document.getElementById('devuserdiv' + nr).classList.add('hide');
        document.getElementById('devpassdiv' + nr).classList.add('hide');
        document.getElementById('setup3div' + nr).classList.remove('hide');
        document.getElementById('setup1div' + nr).classList.remove('hide');
        document.getElementById('setup4div' + nr).classList.remove('hide');
        document.getElementById('setup4text' + nr).innerHTML = 'Max';
        document.getElementById('setup5div' + nr).classList.remove('hide');
        document.getElementById('setup6div' + nr).classList.add('hide');
        document.getElementById('setup7div' + nr).classList.add('hide');
    }
    else if (type == 99) { // ungenutzt
        document.getElementById('specialdiv' + nr).classList.value = 'clear';
        document.getElementById('addressdiv' + nr).classList.add('hide');
        document.getElementById('devuserdiv' + nr).classList.add('hide');
        document.getElementById('devpassdiv' + nr).classList.add('hide');
        document.getElementById('setup3div' + nr).classList.add('hide');
        document.getElementById('setup1div' + nr).classList.add('hide');
        document.getElementById('setup4div' + nr).classList.add('hide');
        document.getElementById('setup5div' + nr).classList.add('hide');
        document.getElementById('setup6div' + nr).classList.add('hide');
        document.getElementById('setup7div' + nr).classList.add('hide');
    }
}

function load_page(seite, init = false) {
    page0get();
    if (seite === "") {
        seite = "page0";
    }
    if (init || aktuelleSeite !== seite) {
        // Menü aktivieren
        if (!userInit && StatusArr[19] !== "Administrator" && (seite === "page3" || seite === "page5")) {
            seite = "page0";
            notification("error", "keine Berechtigung");
        }
        var divs = document.querySelectorAll("#mainmenu .pure-menu-selected"), i;
        for (i = 0; i < divs.length; ++i) {
            divs[i].classList.remove('pure-menu-selected');
        }
        document.querySelector("#m_" + seite).classList.add('pure-menu-selected');

        // Seite aktivieren
        divs = document.querySelectorAll("#content .show");
        for (i = 0; i < divs.length; ++i) {
            divs[i].classList.add('hide');
            divs[i].classList.remove('show');
        }
        document.querySelector("#" + seite).classList.add('show');
        document.querySelector("#" + seite).classList.remove('hide');

        if (!init) {
            switch (aktuelleSeite) {
                case "page0":
                    page0leave();
                    break;
                case "page2":
                    page2leave();
                    break;
            }
            aktuelleSeite = seite;
        }
        switch (aktuelleSeite) {
            case "page0":
                page0start();
                small = "";
                break;
            case "page1":
                loadTimer();
                small = "0";
                break;
            case "page2":
                page2start();
                small = "0";
                break;
            case "page3":
                page3start();
                small = "0";
                break;
            case "page4":
                page4start();
                small = "0";
                break;
            case "page5":
                filesLoad();
                small = "0";
                break;
        }
        /*if (small === "") {
            document.querySelector("#buttonsSmall").classList.add('hide');
        } else {
            document.querySelector("#buttonsSmall").classList.remove('hide');
        }*/
    }
}

function page0start() {
    if (myAjax) {
        myAjaxInterval = setInterval(page0get, 10000);
    }
}
function page0leave() {
    if (myAjaxInterval) {
        clearInterval(myAjaxInterval);
    }
}

function page2start() {
    page2active = setInterval(page2get, 15000);
    page2get();
}
function page2leave() {
    if (page2active) {
        clearInterval(page2active);
    }
}

function page3start() {
    var xmlhttp = new XMLHttpRequest();
    xmlhttp.onreadystatechange = function () {
        if (this.readyState === 4) {
            ajaxOnline = (this.status === 200);
            if (ajaxOnline) {
                var j = JSON.parse(this.responseText);
                document.getElementById("name_dev").value = j.name_dev;
                document.getElementById("ssid").value = j.ssid1;
                document.getElementById("pass").value = j.pass1;
                document.getElementById("timeserver").value = j.timeserver;
                document.getElementById("UpdateServer").value = j.update;
                document.getElementById("AdminName").value = j.name2;
                document.getElementById("AdminPasswort").value = j.pass2;
                document.getElementById("UserName").value = j.name3;
                document.getElementById("UserPasswort").value = j.pass3;

                for (s = 0; s < SPECIAL_MAX; s++) {
                    document.getElementById("name_r" + s).value = j.special[s].name;
                    document.getElementById("address" + s).value = j.special[s].address;
                    document.getElementById("devuser" + s).value = j.special[s].devuser;
                    document.getElementById("devpass" + s).value = j.special[s].devpass;
                    for (i = 0; i < SPECIAL_VAL_MAX; i++) {
                        if (i == 0)
                            typeSelect(s, j.special[s]["setup0"]);
                        document.getElementById("setup" + s + "_" + i).value = j.special[s]["setup" + i];
                    }
                }
                notification("success", "Config geladen");
            } else {
                notification("error", "Config: Status " + this.status + " " + this.statusText);
            }
        }
    };
    xmlhttp.open("GET", "config.json", true);
    xmlhttp.setRequestHeader("Content-type", "text/json");
    xmlhttp.send();
}

function page4start() {
    var xmlhttp = new XMLHttpRequest();
    xmlhttp.onreadystatechange = function () {
        if (this.readyState === 4) {
            if (this.status === 200) {
                document.getElementById("logtable").innerHTML = this.responseText;
                notification("success", "Log geladen");
            } else {
                notification("error", "Log.txt: Status " + this.status + " " + this.statusText);
            }
        }
    };
    xmlhttp.open("GET", "log.txt", true);
    xmlhttp.setRequestHeader("Content-type", "text/plain");
    xmlhttp.send();
}
function stateText(state) {
    StatusArr = state.split(";");
    /* Aktivieren und deaktivieren von Schaltern
    divs = document.querySelectorAll(".btn,.btn1,.btn2");
    for (i = 0; i < divs.length; ++i) {
        if (parseInt(StatusArr[i]) == 1) {
            divs[i].innerHTML = "ON";
            divs[i].className = 'btn';
        } else {
            divs[i].innerHTML = "OFF";
            divs[i].className = 'btn1';
        }
    }
    divs = document.querySelectorAll(".btn00,.btn10,.btn20");
    for (i = 0; i < divs.length; ++i) {
        if (parseInt(StatusArr[i]) ) {
            divs[i].className = 'btn00';
        } else {
            divs[i].className = 'btn10';
        }
    }
    divs = document.querySelectorAll(".btn01,.btn11,.btn21");
    for (i = 0; i < divs.length; ++i) {
        if( parseInt(StatusArr[i+12]) ? parseInt(StatusArr[i]) : !parseInt(StatusArr[i]) ) {
            divs[i].className = 'btn11';
        } else {
            divs[i].className = 'btn21';
        }
    }
     * 
     */
    document.getElementById("aktuell").innerHTML = unixTimeToDateTime(StatusArr[0]);
    document.getElementById("version").innerHTML = StatusArr[1];
    document.getElementById("ChipID").innerHTML = StatusArr[2];
    document.getElementById("userstatus").innerHTML = StatusArr[3];
    document.getElementById("NTPok").innerHTML = StatusArr[4];
    document.getElementById("RTCok").innerHTML = StatusArr[5];
    document.getElementById("IOok").innerHTML = StatusArr[6];
    document.getElementById("DISPLAYok").innerHTML = StatusArr[7];
    document.getElementById("NameTimer").innerHTML = StatusArr[8];
    document.title = StatusArr[8];
    for (i = 0; i < 4; ++i) {
        document.getElementById("NameR" + i).innerHTML = StatusArr[10 + i * (SPECIAL_VAL_MAX + 1)];
    }
    if (userInit && StatusArr[19] === 'Administrator') {
        userInit = 0;
        document.querySelector("#m_page3").classList.remove('pure-menu-disabled');
        document.querySelector("#m_page5").classList.remove('pure-menu-disabled');
    }
}
function LesenAjax() {
    if (this.readyState === 4) {
        ajaxOnline = (this.status === 200);
        var divs, i;
        if (ajaxOnline) {
            stateText(myAjax.responseText);
        } else {
            notification("error", "Schalteraktualisierung: Status " + this.status + " " + this.statusText);
            divs = document.querySelectorAll(".btn,.btn1,.btn2");
            for (i = 0; i < divs.length; ++i) {
                divs[i].className = 'btn2';
            }
            divs = document.querySelectorAll(".btn00,.btn10,.btn20");
            for (i = 0; i < divs.length; ++i) {
                divs[i].className = 'btn20';
            }
        }
    }
}

function Schreiben(num) {
    if (document.getElementsByTagName("input")[num - 1].checked)
        UrlGet = Url + "schalte.php?Relay=" + num + "&On=0";
    else
        UrlGet = Url + "schalte.php?Relay=" + num + "&On=1";
    myAjax.open("GET", UrlGet, true);
    myAjax.send();
}

function Taster(k) {
    var Butt = document.getElementById("button" + k);
    if ((Butt.innerHTML) === "ON") {
        var schalte = '0';
    } else {
        var schalte = '1';
    }
    UrlGet = Url + "schalte.php?Relay=" + (k + 1) + "&On=" + schalte;
    myAjax.open("GET", UrlGet, true);
    myAjax.send();
}

function page0get() {
    UrlGet = Url + "state.json";
    myAjax.open("GET", UrlGet, true);
    myAjax.send();
}
function unixTimeToDateTime(unixTimestamp) {
    const dateObj = new Date(unixTimestamp * 1000); // convert to milliseconds
    var year = dateObj.getFullYear();
    var month = (dateObj.getMonth() + 1); // add 1, since the first month is 0
    var day = dateObj.getDate();
    var hours = dateObj.getHours();
    var minutes = dateObj.getMinutes();
    var seconds = dateObj.getSeconds();
    if (day < 10) day = "0" + day;
    if (month < 10) month = "0" + month;
    if (hours < 10) hours = "0" + hours;
    if (minutes < 10) minutes = "0" + minutes;
    if (seconds < 10) seconds = "0" + seconds;

    // return in a formatted string
    return `${day}.${month}.${year}&nbsp;${hours}:${minutes}:${seconds}`;
}

function page2show(){
    var s, i, m, txt = "";
    if (myObj.sum) {
        txt += "<table border='1'>";
        txt += "<thead><tr><th>Gerät</th><th>Name</th><th>Aktualisierung</th><th>Watt Ist</th><th>Watt Soll</th><th>Watt Max</th><th>Limit Ist</th><th>Limit Soll</th></tr></thead><tbody>";
        for (s in myObj.sum) {
            txt += "<tr class='liveR'><td>Regelung</td><td>" + myObj.sum[s].name +
                "</td><td>" + unixTimeToDateTime(myObj.sum[s].ts_last_success) +
                "</td><td>" + myObj.sum[s].powerUsed +
                "</td><td>" + myObj.sum[s].soll +
                "</td><td>" + myObj.sum[s].install +
                "</td><td>&nbsp;" +
                "</td><td>" + myObj.sum[s].powerLimit +
                "</td></tr>";
            for (i in myObj.dtu) {
                if (myObj.dtu[i].sum == s)
                    txt += "<tr class='liveD'><td>DTU</td><td>" + myObj.dtu[i].version +
                        "</td><td>" + unixTimeToDateTime(myObj.dtu[i].ts_now) +
                        "</td><td>" + myObj.dtu[i].ist +
                        "</td><td>&nbsp;" +
                        "</td><td>" + myObj.dtu[s].max +
                        "</td><td>&nbsp;" +
                        "</td><td>&nbsp;" +
                        "</td></tr>";
            }
            for (i in myObj.inverter) {
                if (myObj.inverter[i].sum == s)
                    txt += "<tr class='liveW'><td>Wechselrichter</td><td>" + myObj.inverter[i].name +
                        "</td><td>" + unixTimeToDateTime(myObj.inverter[i].ts_last_success) +
                        "</td><td>" + myObj.inverter[i].P_AC +
                        "</td><td>" + (myObj.inverter[i].power_limit_set * myObj.inverter[i].max / 100) +
                        "</td><td>" + myObj.inverter[i].max +
                        "</td><td>" + myObj.inverter[i].power_limit_read +
                        "</td><td>" + myObj.inverter[i].power_limit_set +
                        "</td></tr>";
            }
            for (i in myObj.battery) {
                if (myObj.battery[i].sum == s)
                    txt += "<tr class='liveB'><td>Batterie</td><td>" + myObj.battery[i].name +
                        "</td><td>" + unixTimeToDateTime(myObj.battery[i].ble.ts_last_03) +
                        "</td><td>" + (myObj.battery[i].ble.powerOut1 + myObj.battery[i].ble.powerOut2) +
                        "</td><td>" + (myObj.battery[i].ble.pvPower1 + myObj.battery[i].ble.pvPower2) +
                        "</td><td>" + myObj.battery[i].max +
                        "</td><td>" + myObj.battery[i].ble.soc +
                        "</td><td>" + (100-myObj.battery[i].ble.dod_level) +
                        "</td></tr>";
            }
            for (m in myObj.meter) {
                if (myObj.meter[m].sum == s)
                    txt += "<tr class='liveM'><td>Stromzähler</td><td>" + myObj.meter[m].name +
                        "</td><td>" + unixTimeToDateTime(myObj.meter[m].ts_last_success) +
                        "</td><td>" + myObj.meter[m].power +
                        "</td><td>&nbsp;" +
                        "</td><td>&nbsp;" +
                        "</td><td>&nbsp;" +
                        "</td><td>&nbsp;" +
                        "</td></tr>";
            }
        }
        txt += "</tbody></table>";
        document.getElementById("page2content").innerHTML = txt;
    }
    if (myObj.state) {
        stateText(myObj.state);
    }
}

function page2ready(){
    if (this.readyState === 4 && this.status === 200) {
        /*
        if (this.responseText.startsWith('{"dtu":'))
            myObj.dtu = JSON.parse(this.responseText);
        if (this.responseText.startsWith('{"inverter":'))
            myObj.inverter = JSON.parse(this.responseText);
        if (this.responseText.startsWith('{"meter":'))
            myObj.meter = JSON.parse(this.responseText);
        if (this.responseText.startsWith('{"battery":'))
            myObj.battery = JSON.parse(this.responseText);
        if (this.responseText.startsWith('{"sum":'))
            myObj.sum = JSON.parse(this.responseText);
        if (this.responseText.startsWith('{"state":'))
            myObj.state = JSON.parse(this.responseText);*/
        myObj = JSON.parse(this.responseText);
        page2show();
    }
}

function page2get() {
    //UrlGet = Url + "values.json?type=d";
    var xmlhttp = new XMLHttpRequest();
    xmlhttp.open("GET", "values.json");
    xmlhttp.onreadystatechange = page2ready;
    xmlhttp.send();
    /*xmlhttp.open("GET", "values.json?type=i");
    //xmlhttp.onreadystatechange = page2ready;
    xmlhttp.send();
    xmlhttp.open("GET", "values.json?type=m");
    //xmlhttp.onreadystatechange = page2ready;
    xmlhttp.send();
    xmlhttp.open("GET", "values.json?type=b");
    //xmlhttp.onreadystatechange = page2ready;
    xmlhttp.send();
    xmlhttp.open("GET", "values.json?type=s");
    //xmlhttp.onreadystatechange = page2ready;
    xmlhttp.send();*/
}


function DatensatzBilden(Id = "") {
    var st = document.getElementById('dat' + Id).value;
    if (st === "")
        st = "1.1.0";
    var stSplit = st.split(".");
    var szeit = document.getElementById('zeit' + Id).value;
    var szeitSplit = szeit.split(":");
    var jahr = parseInt(stSplit[2], 10);
    if (jahr < 1000)
        jahr += 2000;
    var dt = new Date(Date.UTC(jahr, parseInt(stSplit[1] - 1, 10), parseInt(stSplit[0], 10), parseInt(szeitSplit[0], 10), parseInt(szeitSplit[1], 10), parseInt(szeitSplit[2], 10)));
    var Link = "settimer.php";
    Link += "?art=" + document.getElementById('art' + Id).value;
    Link += "&zeit=";
    Link += dt.valueOf() / 1000;
    Link += "&relais=";
    Link += document.getElementById('relais' + Id).value;
    Link += "&ein=" + document.getElementById('on' + Id).value;
    Link += "&aktiv=" + document.getElementById('aktiv' + Id).checked;
    Link += "&nachholen=" + document.getElementById('nachholen' + Id).checked;
    if (Id <= 250)
        Link += "&id=" + Id;
    self.location.href = Link;
}
function load() {
    var obj, dbParam, xmlhttp, myObj, x, txt = "";
    obj = { "table": "customers", "limit": 20 };
    dbParam = JSON.stringify(obj);
    xmlhttp = new XMLHttpRequest();
    xmlhttp.onreadystatechange = function () {
        if (this.readyState == 4 && this.status == 200) {
            myObj = JSON.parse(this.responseText);
            txt += "<table border='1'>";
            for (x in myObj) {
                txt += "<tr><td>" + myObj[x].name + "</td></tr>";
            }
            txt += "</table>";
            document.getElementById("demo").innerHTML = txt;
        }
    };
    xmlhttp.open("POST", "jsonTimer.php", true);
    xmlhttp.setRequestHeader("Content-type", "application/x-www-form-urlencoded");
    xmlhttp.send("x=" + dbParam);

}
function indexArt(a) {
    switch (a) {
        case "2":
            return 0;
        case "1":
            return 1;
        case "4":
            return 2;
        case "5":
            return 3;
        case "6":
            return 4;
        case "7":
            return 5;
        case "8":
            return 6;
        case "9":
            return 7;
        case "3":
            return 8;
        case "10":
            return 9;
        case "11":
            return 10;
        case "12":
            return 11;
        default:
            return -1;
    }
}
function myCreateFunction(a) {
    var table = document.getElementById("timertable");
    var row = table.insertRow(-1);
    if (a.id <= 250) {
        row.insertCell(0).innerHTML = a.id;
        row.insertCell(1).innerHTML = a.date_n;
    } else {
        row.insertCell(0).innerHTML = "";
        row.insertCell(1).innerHTML = "";
    }
    row.insertCell(2).innerHTML = "<input type=\"checkbox\" name=\"Aktiv\" id=\"aktiv" + a.id + "\" value=\"1\" " + (a.aktiv == "1" ? "checked=\"checked\"" : "") + ">";
    row.insertCell(3).innerHTML = "<input type=\"checkbox\" name=\"Nachholen\" id=\"nachholen" + a.id + "\" value=\"1\" " + (a.nachholen == "1" ? "checked=\"checked\"" : "") + ">";
    row.insertCell(4).innerHTML =
        "<select name=\"Art\" id=\"art" + a.id + "\" size=\"1\" style=\"text-align:center;\">" +
        "<option value=\"2\">Täglich</option><option value=\"1\">Einmalig</option><option value=\"4\">Montags</option><option value=\"5\">Dienstags</option>" +
        "<option value=\"6\">Mittwochs</option><option value=\"7\">Donnerstags</option><option value=\"8\">Freitags</option><option value=\"9\">Samstags</option>" +
        "<option value=\"3\">Sonntags</option><option value=\"10\">Wochenende/Feiertag</option><option value=\"11\">Mo-Fr</option><option value=\"12\">Feiertage</option></select>";
    document.getElementById("art" + a.id).selectedIndex = indexArt(a.art);
    row.insertCell(5).innerHTML = "<input  name=\"Datum\" id=\"dat" + a.id + "\" value=\"" + a.date + "\" type=\"text\" size=\"10\" maxlength=\"10\" style=\"text-align:center;\">";
    row.insertCell(6).innerHTML = "<input  name=\"Zeit\" id=\"zeit" + a.id + "\" value=\"" + a.time + "\" type=\"text\" size=\"8\" maxlength=\"8\" style=\"text-align:center;\">";
    row.insertCell(7).innerHTML = "<select name=\"Relais\" id=\"relais" + a.id + "\" size=\"1\">" +
        "<option value=\"1\">Relais Nr.: 1</option>" +
        "<option value=\"2\">Relais Nr.: 2</option>" +
        "<option value=\"3\">Relais Nr.: 3</option>" +
        "<option value=\"4\">Relais Nr.: 4</option></select>";
    document.getElementById("relais" + a.id).selectedIndex = a.relais - 1;
    row.insertCell(8).innerHTML = "<select name=\"On\" id=\"on" + a.id + "\" size=\"1\"><option value=\"0\">OFF</option> <option value=\"1\">ON</option></select>";
    document.getElementById("on" + a.id).selectedIndex = a.ein;
    if (a.id == "251" || a.id == "252") {
        row.insertCell(9).innerHTML = "<a href=\"#\" onclick=\"DatensatzBilden(" + a.id + ");return false;\">Neu</a>";
        row.insertCell(10).innerHTML = "";
    } else {
        row.insertCell(9).innerHTML = "<a href=\"#\" onclick=\"DatensatzBilden(" + a.id + ");return false;\">Anpassen</a>";
        row.insertCell(10).innerHTML = "<a href=\"delete.php?Nr=" + a.id + "\" >Löschen</a>";
    }
}

function myDeleteFunction() {
    document.getElementById("timertable").deleteRow(0);
}

function loadTimer() {
    var table = document.getElementById("timertable");
    table.innerHTML = "";
    var row = table.createTHead().insertRow(-1);
    row.innerHTML = "<th>Index</th><th>Nächstes Ereignis</th><th>Aktiv</th><th>Nachholen</th><th>Art</th><th>Datum</th><th>Zeit</th><th>Relais</th><th>On/Off</th><th>Aktion</th><th>Löschen</th>";
    myCreateFunction({ id: "251", aktiv: "1", nachholen: "1", art: "-1", relais: -1, on: -1, time: "", date: "", date_n: "" });

    var xmlhttp = new XMLHttpRequest();

    xmlhttp.onreadystatechange = function () {
        if (this.readyState == 4 && this.status == 200) {
            var j = JSON.parse(this.responseText);
            for (var x in j) {
                if (j[x].id == "Z") {
                    document.getElementById("aktuell").innerHTML = j[x].date + "&nbsp;" + j[x].time;
                } else
                    myCreateFunction(j[x]);
            }
            myCreateFunction({ id: "252", aktiv: "1", nachholen: "1", art: "-1", relais: -1, on: -1, time: "", date: "", date_n: "" });
        }
    };
    xmlhttp.open("POST", "timer.json", true);
    xmlhttp.setRequestHeader("Content-type", "application/x-www-form-urlencoded");
    xmlhttp.send();
}
function rebootButton() {
    notification("success", "Reboot ausgelöst ...");
    var xmlhttp = new XMLHttpRequest();
    xmlhttp.onreadystatechange = function () {
        if (this.readyState == 4 && this.status == 200) {
            notification("success", this.responseText);
        }
    };
    xmlhttp.open("GET", "/reboot", true);
    xmlhttp.send();
  }
  
function filesLoad() {
    var table = document.getElementById("filetable");
    table.innerHTML = "";
    var row = table.createTHead().insertRow(-1);
    row.innerHTML = "<th>Dateiname</th><th>Typ</th><th>Größe</th><th>Download</th><th>Löschen</th>";

    var xmlhttp = new XMLHttpRequest();

    xmlhttp.onreadystatechange = function () {
        if (this.readyState == 4 && this.status == 200) {
            var table = document.getElementById("filetable");
            var tbody = table.createTBody();
            var j = JSON.parse(this.responseText);
            for (var x in j) {
                if (j[x].type === "file") {
                    var row = tbody.insertRow(-1);
                    row.insertCell(0).innerHTML = j[x].name;
                    row.insertCell(1).innerHTML = j[x].type;
                    row.insertCell(2).innerHTML = j[x].size;
                    row.insertCell(3).innerHTML = "<a href=\"/" + j[x].name + "\" download>Download</a>";
                    row.insertCell(4).innerHTML = "<a href=\"#page5\" onclick=\"filesDelete('" + j[x].name + "');return false;\">Löschen</a>";
                } else {

                }
            }
        }
    };
    xmlhttp.open("GET", "list?dir=/", true);
    xmlhttp.setRequestHeader("Content-type", "text/json");
    xmlhttp.send();
}
function filesDelete(a) {
    var xmlhttp = new XMLHttpRequest();
    xmlhttp.onreadystatechange = function () {
        if (this.readyState == 4) {
            if (this.status == 200) {
                //var j = JSON.parse(this.responseText);
                notification("success", "Datei gelöscht");
            } else {
                notification("error", "Datei nicht gelöscht: Status " + this.status + " " + this.statusText);
            }
        }
    };
    xmlhttp.open("GET", "delete?file=/" + a, true);
    xmlhttp.setRequestHeader("Content-type", "text/json");
    xmlhttp.send();
}
function filesUpload() {
    var upload = document.getElementById("uploadfile");
    var formData = new FormData();
    formData.append("upload", upload.files[0]);
    var xmlhttp = new XMLHttpRequest();
    xmlhttp.onreadystatechange = function () {
        if (this.readyState == 4) {
            if (this.status == 200) {
                //var j = JSON.parse(this.responseText);
                notification("success", "Datei geladen");
                document.getElementById("uploadfile").innerHTML = '';
            } else {
                notification("error", "Datei nicht geladen: Status " + this.status + " " + this.statusText);
            }
        }
    };
    xmlhttp.open("POST", "upload.json", true);
    //xmlhttp.setRequestHeader("Content-type", "multipart/form-data");
    //xmlhttp.setRequestHeader("Content-type", "false");
    xmlhttp.send(formData);
}

function notification(state, message) {
    var notif = document.getElementById("notification");
    notifId++;
    var n1 = document.createElement('notif_' + notifId);
    notif.appendChild(n1);
    n1.innerHTML = message;
    n1.classList.add('pure-button');
    n1.classList.add('button-' + state);
    n1.classList.add('w3-animate-fading');
    var n1Timeout = setTimeout(function () {
        n1.parentNode.removeChild(n1);
        //n1.classList.remove('w3-animate-fading');
    }, 10000);
    n1.addEventListener("click", function () {
        //n1.classList.remove('w3-animate-fading');
        clearTimeout(n1Timeout);
        n1.parentNode.removeChild(n1);
    });
    //notif.classList.add('w3-animate-fading');
}