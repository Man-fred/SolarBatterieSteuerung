#include <Arduino.h>

static const char upload_htm[] PROGMEM = R"=====(<!doctype HTML>
<html lang="en">
<head>
<title>ESP32 UPLOAD</title>
<meta charset="utf-8">
<meta http-equiv="Content-Type" content="text/html; charset=utf-8" />
<link rel="icon" href="data:;base64,iVBORw0KGgo=">  <!--prevent favicon requests-->
<style>
#uploadForm {
    width:350px;
    margin:0 auto;
    background-color:beige;
    text-align:center;
    border:solid 1px black;
}
#title{
    text-align:center;
    margin:5px;
}
#fileSelection{
    height:50px;
    border:solid 1px black;
}
#uploadProgressBar{
    margin:10px;
}
#uploadFileButton{
    width:100px;
    height:30px;
    margin:10px;
}
#statusText{
    margin:5px;
}
#credentials{
    margin:2px;
    display:flex;
}
#username, #password {
    width:48%;
    margin:5px;
    text-align:center;
}
</style>
</head>
<body>
    <form id="uploadForm" enctype="multipart/form-data">
        <p id="title">Simple authenticated upload example<br>for ESP32 and ESPAsyncWebServer</p>
        <p id="credentials">
            <input type="text" id="username" name="username" placeholder="username">
            <input type="password" id="password" name="password" placeholder="password">
        </p>
        <input id="fileSelection" name="file" type="file" />
        <progress id="uploadProgressBar" value="0" max="0"></progress>
        <p id="statusText">&nbsp;</p>
        <input id="uploadFileButton" type="submit" value="Upload" disabled="disabled"/>
    </form>
<script type="text/javascript">

function startUpload() {
    let request = new XMLHttpRequest();

    request.onerror = function() {
        window.statusText.innerHTML = 'ERROR: http request failed!';
    }

    request.ontimeout = function() {
        window.statusText.innerHTML = 'ERROR: Connection timeout!';
    }

    request.onreadystatechange = function() {
        if (this.status == 404) {
            window.statusText.innerHTML = '404 - Upload URL not found on server';
            window.fileSelection.value = '';
        }

        if (this.readyState == 4) {
            switch (this.status) {
                case 200 :
                    window.statusText.innerHTML = 'Upload succes!';
                    window.fileSelection.value = '';
                    window.fileSelection.disabled = false;
                    window.uploadFileButton.disabled = true;
                    break;
                case 400 :
                    window.statusText.innerHTML = this.responseText;
                    window.fileSelection.disabled = false;
                    break;
                case 401 :
                    window.statusText.innerHTML = 'Enter valid username and password';
                    window.fileSelection.disabled = false;
                    window.uploadFileButton.disabled = false;
                    break;
                default : console.log('http result code: ' + this.status);
            }
        }
    };

    request.upload.addEventListener('progress', function(event) {
        const percent = (event.loaded / event.total) * 100;
        window.statusText.innerHTML = Math.round(percent) + '% uploaded. Please wait...';
        let progress = window.uploadProgressBar;
        progress.setAttribute('value', event.loaded);
        progress.setAttribute('max', event.total);
    });

    request.addEventListener('load', function() {
        let progress = window.uploadProgressBar;
        progress.setAttribute('value', 0);
        progress.setAttribute('max', 0);
    });

    request.open('POST', '/upload.htm');

    request.setRequestHeader('Authorization', 'Basic ' + btoa(window.username.value + ":" + window.password.value));

    const file = window.fileSelection.files[0];
    request.setRequestHeader('FileSize', file.size);

    let data = new FormData();
    data.append('file', file);

    window.statusText.innerHTML = 'Initializing upload...';

    request.send(data);
}

document.getElementById('fileSelection').addEventListener('change', function() {
    window.uploadFileButton.disabled = this.files[0] ? false : true;
    window.statusText.innerHTML = '&nbsp;';
});

document.getElementById('uploadFileButton').addEventListener('click', function(event){
    event.preventDefault();
    this.disabled = true;
    window.fileSelection.disabled = true;
    startUpload();
});

</script>
</body>
</html>
)=====";
