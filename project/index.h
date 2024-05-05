const char webpage[] PROGMEM = R"=====(
<!DOCTYPE html>
<html>
<head>
  <meta charset="utf-8">
  <style type="text/css">
    .button {
      background-color: #4CAF50; /* Green */
      border: none;
      color: white;
      padding: 15px 32px;
      text-align: center;
      text-decoration: none;
      display: inline-block;
      font-size: 16px;
    }
  </style>
</head>
<body style="background-color: #f9e79f">
  <center>
    <div>
      <h1>Количество людей в помещении</h1>
      <h4>Управление максимальным количеством людей в помещении <h4>
      <button class="button" onclick="send(1)">+</button>
      <button class="button" onclick="send(0)">-</button><BR>
    </div>
    <br>
    <div>
      <h2>
        Сейчас в помещении: <span id="adc_val">0</span>людей<br> <br>
        Максимум людей: <span id="state">NA</span>
      </h2>
    </div>
    <script>
      function send(led_sts) {
        var xhttp = new XMLHttpRequest();
        xhttp.onreadystatechange = function() {
          if (this.readyState == 4 && this.status == 200) {
            document.getElementById("state").innerHTML = this.responseText;
          }
        };
        xhttp.open("GET", "led_set?state="+led_sts, true);
        xhttp.send();
      }
      setInterval(function() {
        getData();
      }, 2000); 
      function getData() {
        var xhttp = new XMLHttpRequest();
        xhttp.onreadystatechange = function() {
          if (this.readyState == 4 && this.status == 200) {
            document.getElementById("adc_val").innerHTML =
            this.responseText;
          }
        };
        xhttp.open("GET", "adcread", true);
        xhttp.send();
      }
    </script>
  </center>
</body>
</html>
)=====";
