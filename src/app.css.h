static const char app_css[] PROGMEM = R"=====(
/*
Farbschema generiert auf 
https://www.webmart.de/web/harmonische-farben#1F8DD6:t:d:g:d 
Kontraste:
.myColor-1 { color: #007ea6; } // Deep Cerulean
.myColor-2 { color: #005874; } // Orient
.myColor-3 { color: #bff0ff; } // Onahau
.myColor-4 { color: #80e0ff; } // Anakiwa

.myColor-5 { color: #ff7400; } // Flush Orange
.myColor-6 { color: #b35100; } // Rose of Sharon
.myColor-7 { color: #ffdcbf; } // Negroni
.myColor-8 { color: #ffba80; } // Macaroni and Cheese

.myColor-9 { color: #ffc000; } // Amber
.myColor-10 { color: #b38600; } // Pirate Gold
.myColor-11 { color: #ffefbf; } // Egg White
.myColor-12 { color: #ffe080; } // Salomie

.myColor-13 { color: #270ca6; } // Blue Gem
.myColor-14 { color: #1b0874; } // Deep Blue
.myColor-15 { color: #cec4ff; } // Melrose
.myColor-16 { color: #9e89ff; } // Heliotrope
*/
:root {
  --blue1: #1F8DD6;
  --blue2: #80e0ff;
  --table1: #b38600;
  --table2: #ffc000;
  --table3: #ffe080;
  --table4: #ffefbf;
  --table5: #ffdcbf;
  --table6: #bff0ff;
  --white: #ffffff;
  --black: #000000;
}
* { box-sizing: border-box; }
body {
    color: #000;
}
#main {
    height: 100%;
}

.row:after {
  content: "";
  display: table;
  clear: both;
}
 
.column.full,
.column.two-thirds,
.column.half,
.column.one-third,
.column.one-fourth {
  float: none;
  margin: 0;
  width: 100%;
}
label {
    float: left;
    clear: left;
}
.nowrap {
    white-space: nowrap;
    overflow: hidden;
    float: left;
    clear: left;
}
.left {
    float: left;
}
.right {
        flex-grow: 1;
}
.clear{
    display:flex;
    clear: both;
}
.i500 {
    clear: left;
    width: 100%;
}
@media screen and (min-width: 400px) {
    .column.two-thirds {  width: 65%; float: left;  margin-left: 5%;}
    .column.half {  width: 47.5%; float: left;  margin-left: 5%;}
    .column.one-third {  width: 30%; float: left;  margin-left: 5%;}
    .column.one-third-640 {  width: 63%; float: left;  margin-left: 5%;}
    .column.one-fourth {  width: 21.25%; float: left;  margin-left: 5%;}
}
@media screen and (min-width: 500px) {
    .nowrap { clear: none;} 
    .i500 { width:50%; clear: none;} 
}
@media screen and (min-width: 640px) {
    .column.one-third-640 {  width: 30%; float: left;  margin-left: 5%;}
}
@media screen and (min-width: 960px) {
    .column.one-third-960 {  width: 30%; float: left;  margin-left: 5%;}
}

.column:first-child {
  margin-left: 0;
}

.pure-img-responsive {
    max-width: 100%;
    height: auto;
}
.pure-g {
  align-content: flex-start;
  display: flex;
  flex-flow: row wrap;
}
.pure-u-1-9 {width: 11%;}
.pure-u-2-9 {width: 22%;}
.pure-u-3-9 {width: 33%;}
.pure-u-4-9 {width: 44%;}
.pure-u-5-9 {width: 55%;}
.pure-u-6-9 {width: 66%;}
.pure-u-7-9 {width: 77%;}
.pure-u-8-9 {width: 88%;}
.pure-u-9-9 {width: 99%;}
/*
Add transition to containers so they can push in and out.
*/
#layout,
#menu,
.menu-link {
    -webkit-transition: all 0.2s ease-out;
    -moz-transition: all 0.2s ease-out;
    -ms-transition: all 0.2s ease-out;
    -o-transition: all 0.2s ease-out;
    transition: all 0.2s ease-out;
}

/*
This is the parent `<div>` that contains the menu and the content area.
*/
#layout {
    position: relative;
    left: 0;
    padding-left: 0;
        height: 100%;
}
#layout.active #menu {
    left: 150px;
    width: 150px;
}

#layout.active .menu-link {
    left: 150px;
}
/*
The content `<div>` is where all your content goes.
*/
.content {
    margin: 0 auto;
    padding: 0 2em;
    max-width: 800px;
    margin-bottom: 50px;
    line-height: 1.6em;
}

.content-subhead {
    margin: 50px 0 20px 0;
    font-weight: 300;
    color: #888;
}
.linksmitte {
    vertical-align: middle;
    text-align: left;
}

/*
The `#menu` `<div>` is the parent `<div>` that contains the `.pure-menu` that
appears on the left side of the page.
*/

#menu {
    margin-left: -150px; /* "#menu" width */
    width: 150px;
    position: fixed;
    top: 0;
    left: 0;
    bottom: 0;
    z-index: 1000; /* so the menu or its navicon stays above all content */
    background: #191818;
    overflow-y: auto;
    -webkit-overflow-scrolling: touch;
}
/*
All anchors inside the menu should be styled like this.
*/
#menu a {
        font-weight: bold;
       text-decoration: none;
        display: block;
    color: #999;
    border: none;
    padding: 0.6em 0 0.6em 0.6em;
}

/*
Remove all background/borders, since we are applying them to #menu.
*/
#menu ul {
    border: none;
    background: transparent;
    padding: 0;
}

/*
Add that light border to separate items into groups.
*/
#menu  ul,
#menu  .menu-item-divided {
    border-top: 4px solid #333;
}
#menu  .menu-info {color: #0CC;font-size: 0.8em;height: 1.3em;}
/*
Change color of the anchor links on hover/focus.
*/
#menu li {
    height: 2em;
    list-style-type: none;
}
#menu li a:hover {
    background: #ff0000;
}
    
#menu li a:focus {
    background: #333;
}

/*
This styles the selected menu item `<li>`.
*/
#menu .pure-menu-selected,
#menu .pure-menu-heading {
    background: var(--blue1);
}
/*
This styles a link within a selected menu item `<li>`.
*/
#menu .pure-menu-selected a {
    color: var(--white);
}

/*
This styles the menu heading.
*/
#menu .pure-menu-heading {
    font-size: 110%;
    color: var(--white);
    margin: 0;
}

/* -- Dynamic Button For Responsive Menu -------------------------------------*/

/*
The button to open/close the Menu is custom-made and not part of Pure. Here's
how it works:
*/

/*
`.menu-link` represents the responsive menu toggle that shows/hides on
small screens.
*/
.menu-link {
    position: fixed;
    display: block; /* show this only on small screens */
    top: 0;
    left: 0; /* "#menu width" */
    background: var(--black);
    background: rgba(0,0,0,0.7);
    font-size: 10px; /* change this value to increase/decrease button size */
    z-index: 10;
    width: 44px;
    height: 44px;
    padding: 2.1em 1.6em;
}

.menu-link:hover,
.menu-link:focus {
    background: var(--black);
}

.menu-link span {
    position: relative;
    display: block;
}

.menu-link span,
.menu-link span:before,
.menu-link span:after {
    background-color: var(--white);
    width: 100%;
    height: 0.2em;
}

.menu-link span:before,
.menu-link span:after {
    position: absolute;
    margin-top: -0.6em;
    content: " ";
}

.menu-link span:after {
    margin-top: 0.6em;
}

h1 {
    text-align: right;
    margin: 0 1em 0 1em;
    overflow:hidden;
}
/* -- Responsive Styles (Media Queries) ------------------------------------- */

/*
Hides the menu at `48em`, but modify this based on your app's needs.
*/
@media (min-width: 48em) {

    .header,
    .content {
        padding-left: 2em;
        padding-right: 2em;
    }

    #layout {
        padding-left: 150px; /* left col width "#menu" */
        left: 0;
    }
    #menu {
        left: 150px;
    }

    .menu-link {
        position: fixed;
        left: 150px;
        display: none;
    }

    #layout.active .menu-link {
        left: 150px;
    }
    h1 {
        text-align: center;
    }
}

@media (max-width: 48em) {
    /* Only apply this when the window is small. Otherwise, the following
    case results in extra padding on the left:
        * Make the window small.
        * Tap the menu to trigger the active state.
        * Make the window large again.
    */
    #layout.active {
        position: relative;
        left: 150px;
    }
}

/* 
    Created on : 09.06.2017, 19:49:44
    Author     : Manfred
*/

.custom-restricted-width {
    /* To limit the menu width to the content of the menu: */
    display: inline-block;
    /* Or set the width explicitly: */
    /* width: 10em; */
}

.hide {display:none ; }
.show {display:block ; }

.white-space-pre {
    white-space: pre-wrap;
}

table
{
    font-family:"Trebuchet MS", Arial, Helvetica, sans-serif;
    /*vertical-align: */
    border-collapse:collapse;
}
table td, table th
{
    font-size:1em;
    border:1px solid var(--table2);
    padding:3px 7px 2px 7px;
}
table th
{
    font-size:1.1em;
    padding-top:5px;
    padding-bottom:4px;
    background-color:var(--table1);
    color:var(--white);
}
table tr.alt td
{
    color:#000000;
    background-color:#EAF2D3;
}
.liveR{background-color:var(--table2);}
.liveD{background-color:var(--table3);}
.liveW{background-color:var(--table4);}
.liveM{background-color:var(--table5);}
.liveB{background-color:var(--table6);}

.btn {
    background: #3498db;
    background-image: -webkit-linear-gradient(top, #3498db, #2980b9);
    background-image: -moz-linear-gradient(top, #3498db, #2980b9);
    background-image: -ms-linear-gradient(top, #3498db, #2980b9);
    background-image: -o-linear-gradient(top, #3498db, #2980b9);
    background-image: linear-gradient(to bottom, #3498db, #2980b9);
    -webkit-border-radius: 25;
    -moz-border-radius: 25;
    border-radius: 25px;
    font-family: Arial;
    color: #ffffff;
    width  :  50px;
    height :50px;
    vertical-align: center; 
    text-align: center;
    width  :  50px;
    height :50px;
    font-size: 16px; 
}
.btn:hover {
    background: #3cb0fd;
    background-image: -webkit-linear-gradient(top, #3cb0fd, #3498db);
    background-image: -moz-linear-gradient(top, #3cb0fd, #3498db);
    background-image: -ms-linear-gradient(top, #3cb0fd, #3498db);
    background-image: -o-linear-gradient(top, #3cb0fd, #3498db);
    background-image: linear-gradient(to bottom, #3cb0fd, #3498db);
    text-decoration: none;
}
.btn1 {
    -webkit-border-radius: 25;
    -moz-border-radius: 25;
    border-radius: 25px;
    font-family: Arial;
    color: #ffffff;
    width  :  50px;
    height :50px;
    vertical-align: center; 
    text-align: center;
    width  :  50px;
    height :50px;
    font-size: 16px;
    background: #3cb0fd;
    background-image: -webkit-linear-gradient(top, #ed13e9, #de1843);
    background-image: -moz-linear-gradient(top, #ed13e9, #de1843);
    background-image: -ms-linear-gradient(top, #ed13e9, #de1843);
    background-image: -o-linear-gradient(top, #ed13e9, #de1843);
    background-image: linear-gradient(to bottom, #ed13e9, #de1843);
    text-decoration: none;  
}
.btn1:hover {
    background: #3498db;
    background-image: -webkit-linear-gradient(top, #de1843, #ed13e9);
    background-image: -moz-linear-gradient(top, #de1843, #ed13e9);
    background-image: -ms-linear-gradient(top, #de1843, #ed13e9);
    background-image: -o-linear-gradient(top, #de1843, #ed13e9);
    background-image: linear-gradient(to bottom, #de1843, #ed13e9); 
}
.btn2 {
    -webkit-border-radius: 25;
    -moz-border-radius: 25;
    border-radius: 25px;
    font-family: Arial;
    color: #ffffff;
    width  :  50px;
    height :50px;
    vertical-align: center; 
    text-align: center;
    width  :  50px;
    height :50px;
    font-size: 16px;
    background: #b6b6b6;
    text-decoration: none;  
}
.btn00, .btn01, .button-success {
    background: rgb(28, 184, 65); /* this is a green */
}
.btn10, .btn11, .button-error {
    background: rgb(202, 60, 60); /* this is a maroon */
}
.btn20, .btn21 {
    background: #b6b6b6;
    /*color:#b6b6b6;*/
}
.btn00, .btn10, .btn20, .btn01, .btn11, .btn21 {
    width:4em;
    height:1.5em
}
/* pure-buttons */
.button-success,
.button-error,
.button-warning,
.button-secondary {
    color: white;
    border-radius: 4px;
    text-shadow: 0 1px 1px rgba(0, 0, 0, 0.2);
}

.button-warning {
    background: rgb(223, 117, 20); /* this is an orange */
}

.button-secondary {
    background: rgb(66, 184, 221); /* this is a light blue */
}


/* notyf */
#notification{
    position: absolute;
    width: 250px;
    /*height: 45px;*/
    top: 0;
    right: 0;
}

.w3-animate-fading{
    /*visibility: visible !important;*/
    position: relative;
    width: 100%;
    margin: 1px;
    animation:fading 10s infinite
}@keyframes fading{0%{opacity:0}20%{opacity:1}80%{opacity:1}100%{opacity:0}}
/* fester Header / Footer */
html, body {
    height:100%;
    padding:0;
    overflow:hidden;
    margin: 0;
}
header, nav, section, footer {
    display: block !important;
    /*    -moz-box-sizing: border-box;
        -webkit-box-sizing: border-box;
        box-sizing: border-box;*/
} 
header {
    top:0px;
    position:fixed !important;
    position:absolute;
    padding-top: 0 !important;
    height:44px;
    width:100%;
    background-color: var(--blue1);
}
nav {
    bottom:0px;
    height:30%;
    left:0px;
    position:absolute;
    width:100%;
}
section {
    top:54px;
    height:calc(100% - 80px);
    /*height:70vh;*/
    max-width: 1600px !important;
    position:relative;
    overflow:auto;
    width:100%;
}
footer {
    bottom:0px;
    position:fixed !important;
    position:absolute;
    height:25px;
    width:100%;
    background-color: var(--blue1);
}
)=====";