static const char index_htm[] PROGMEM = R"=====(<!DOCTYPE html>
<html manifest="app.manifest">
    <head>
        <title>ESP8266 Nulleinspeisung</title>
        <meta name="viewport" content="width=device-width, initial-scale=1.0">
        <meta content="text/html; charset=UTF-8" http-equiv="content-type">
        <link rel="stylesheet" href="app.css">
        <link rel="icon" type="image/svg+xml" href="data:image/svg+xml,<svg xmlns=%22http://www.w3.org/2000/svg%22 width=%22256%22 height=%22256%22 viewBox=%220 0 100 100%22><rect width=%22100%22 height=%22100%22 rx=%2220%22 fill=%22%231f8dd6%22></rect><text x=%2250%%22 y=%2250%%22 dominant-baseline=%22central%22 text-anchor=%22middle%22 font-size=%2290%22>🌤️</text></svg>" />
    </head>
    <body>
        <div id="layout">
            <a href="#menu" id="menuLink" class="menu-link">
                <!-- Hamburger icon -->
                <span></span>
            </a>

            <div id="menu">
                <div id="mainmenu" class="pure-menu">
                    <a class="pure-menu-heading" href="#">ESP8266</a>

                    <ul class="pure-menu-list">
                        <li id="m_page0" class="pure-menu-item menu-item-divided">
                            <a href="#page0" class="pure-menu-link" onclick="load_page('page0');">Übersicht</a>
                        </li>
                        <li id="m_page1" class="pure-menu-item">
                            <a href="#page1" class="pure-menu-link" onclick="load_page('page1');">Ertrag steuern</a>
                        </li>
                        <li id="m_page2" class="pure-menu-item">
                            <a href="#page2" class="pure-menu-link" onclick="load_page('page2');">Livestatus</a>
                        </li>
                        <li id="m_page3" class="pure-menu-item pure-menu-disabled">
                            <a href="#page3" class="pure-menu-link" onclick="load_page('page3');">Config</a>
                        </li>
                        <li id="m_page4" class="pure-menu-item">
                            <a href="#page4" class="pure-menu-link" onclick="load_page('page4');">Log</a>
                        </li>
                        <li id="m_page5" class="pure-menu-item pure-menu-disabled">
                            <a href="#page5" class="pure-menu-link" onclick="load_page('page5');">Dateisystem</a>
                        </li>
                        <li class=" menu-item-divided"><br /></li>
                    <li class="menu-info">ChipID: <span id="ChipID"></span></li>
                    <li class="menu-info">Firmware Version:</li>
                    <li class="menu-info"><span id="version"></span></li>
                    <li class="menu-info">&copy; Manfred Bielemeier</li>
                    <li class="menu-info">Lizenz:GPL</li>
                    </ul>
                </div>
            </div>
            <div id="main">
                <header class="row">
                    <div class="column full">
                        <h1><span id="NameTimer">Nulleinspeisung</span></h1>
                    </div>
                </header>
                <section id="content" class="content">
                    <div class="row">
                    <div class="nowrap column half">
                        Aktuelle&nbsp;Zeit:&nbsp;<span id="aktuell"></span><br />Berechtigung:&nbsp;<span id="userstatus"></span>
                    </div>
                    <div  class="column half linksmitte">
                        <span id="buttonsSmall" class="nowrap">
                            <button class="btn00">S 1</button>&nbsp;<button class="btn00">S 2</button>&nbsp;<button class="btn00">S 3</button>&nbsp;<button class="btn00">S 4</button>
                        </span><br />
                        <span id="buttonsLED" class="nowrap">
                            <button class="btn21">Led1</button>&nbsp;<button class="btn21">Led2</button>&nbsp;<button class="btn21">Led3</button>&nbsp;<button class="btn21">Led4</button>
                        </span>
                    </div>
                    </div>
                    <div id="page0" class="row page hide">
                        <h2>WLAN Nulleinspeisung</h2>
                        <table style="text-align: left; width: 100%; height: 45px;" border="0" cellpadding="0" cellspacing="0">

                            <tbody>
                                <tr>
                                    <td style="vertical-align: center; text-align: center;"> 
                                        <button id="button0" class="btn" onclick="Taster(0)">ON</button> 
                                    </td>
                                    <td style="vertical-align: center; text-align: center;">  
                                        <button id="button1" class="btn" onclick="Taster(1)">ON</button> 
                                    </td>
                                    <td style="vertical-align: center; text-align: center;">   
                                        <button id="button2" class="btn" onclick="Taster(2)">ON</button> 
                                    </td>
                                    <td style="vertical-align: center; text-align: center;">
                                        <button id="button3" class="btn" onclick="Taster(3)">ON</button>     
                                    </td>
                                </tr>
                                <tr>
                                    <td><span id="NameR0"></span></td>
                                    <td><span id="NameR1"></span></td>
                                    <td><span id="NameR2"></span></td>
                                    <td><span id="NameR3"></span></td>
                                </tr>
                            </tbody>
                        </table>  
                    </div>
                    <div id="page1" class="row page hide">
                        <h2>Timer Verwaltung</h2>
                        <a href="/laden.html" class="pure-button">Timer aktualisieren</a>

                        <table align="center" id="timertable"></table>
                    </div>
                    <div id="page2" class="row page hide">
                        <h2>Live</h2>
                        <div id="page2content"></div>
                    </div>
                    <div id="page3" class="row page hide">
                        <h2>Konfiguration des ESP8266</h2>
                        <form method="post" action="setup.json"  class="pure-form pure-form-aligned">
                            <fieldset>
                                <legend>WLAN-Konfiguration</legend>
                                <div class="pure-control-group">
                                    <label class="i500" for="ssid">WLAN Netzwerk Name</label> 
                                    <input name="ssid" id="ssid" value="" type="text"> 
                                </div>
                                <div class="pure-control-group">
                                    <label class="i500" for="pass">Passwort</label>
                                    <input name="passwort" id="pass" value="" type="text"> 
                                </div>
                                <div class="pure-control-group">
                                    <label class="i500" for="timeserver">Zeitserver</label>
                                    <input name="timeserver" id="timeserver" value="time.nist.gov" type="text"> 
                                </div>
                                <div class="pure-control-group">
                                    <label class="i500" for="UpdateServer">Update-Server</label>
                                    <input name="UpdateServer" id="UpdateServer" value="" type="text"> 
                                </div>
                            </fieldset>
                            <fieldset>
                                <legend>HTTP-Authentifizierung</legend>
                                <div class="pure-control-group">
                                    <label class="i500" for="AdminName">Administrator</label>
                                    <input name="AdminName" id="AdminName" value="" type="text"> 
                                </div>
                                <div class="pure-control-group">
                                    <label class="i500" for="AdminPasswort">Administratorpasswort</label>
                                    <input name="AdminPasswort" id="AdminPasswort" value="" type="text"> 
                                </div>
                                <div class="pure-control-group">
                                    <label class="i500" for="UserName">Benutzer</label>
                                    <input name="UserName" id="UserName" value="" type="text"> 
                                </div>
                                <div class="pure-control-group">
                                    <label class="i500" for="UserPasswort">Benutzerpasswort</label>
                                    <input name="UserPasswort" id="UserPasswort" value="" type="text"> 
                                </div>
                            </fieldset>
                            <fieldset>
                                <legend>Beschreibung</legend>
                                <div class="pure-control-group">
                                    <label class="i500" for="name_dev">Gerätename</label> 
                                    <input name="name_dev" id="name_dev" value="Timer" type="text"> 
                                </div>
                            </fieldset>
                            <fieldset>
                                <legend>Einstellungen</legend>
                                <div id="special_config" style="min-width: 500px"></div>
                            </fieldset>
                            <button name="submit" id="submit_id" value="Senden" type="submit" class="pure-button pure-button-primary">Senden</button>
                        </form>
                    </div>
                    <div id="page4" class="row page hide">
                        <h2>Log-Datei</h2>
                        <a href="/log.txt" class="pure-button"  download>Log herunterladen</a>
                        <a href="/deletelog.php" class="pure-button">Log löschen</a><br/>
                        <code id="logtable" class="white-space-pre"></code>
                    </div>
                    <div id="page5" class="row page hide">
                        <h2>Dateisystem</h2>
                        <button onclick="rebootButton()">Reboot</button>
     <form method='POST' action='/update_client' enctype='multipart/form-data'>
         Firmware:<br>
         <input type='file' accept='.bin,.bin.gz' name='firmware'>
         <input type='submit' value='Update Firmware'>
     </form>
 <div class="flex flex-row">
 <p>OTA Mode</p>
 <select id="otaMode" class="block">
 <option value="fr" selected="">Firmware</option>
 <option value="fs">LittleFS / SPIFFS</option>
 </select>
 </div>
                         <form method='POST' action='/upload' enctype='multipart/form-data' onsubmit="filesUpload();return false;">
                            <input type='file' id='uploadfile'>
                            <input type='submit' value='Upload'>
                        </form>
                        <table align="center" id="filetable"></table>
                        <a href="/update.php" class="pure-button">Firmware-Update über Webserver</a>
                    </div>
                    <div id="page6" class="row page hide">
                        <h2>keine Berechtigung</h2>
                        <p>Für die aufgerufene Funktion haben Sie keine Berechtigung. Bitte loggen Sie sich als Administrator ein.</p>
                    </div>
                    <div id="page7" class="row page hide">
                        <h2>Seite nicht gefunden</h2>
                        <p>Die aufgerufene Seite ist nicht vorhanden. Bitte wählen Sie aus dem Menü eine vorhandene Seite.</p>
                    </div>
                </section>
                <footer id="footer" class="row pure-g">
                        <span id="buttonsSmallX" class="nowrap">
                            NTP:<span id="NTPok"></span>&nbsp;RTC:<span id="RTCok"></span>&nbsp;I2C:<span id="IOok">0</span>&nbsp;Display:<span id="DISPLAYok">0</span>
                        </span>
                </footer>
                <div id="notification"></div>
            </div>
        </div>
        <script src="app.js"></script>
    </body>
</html>
)=====";
