// Captive Portal
#include <AsyncTCP.h>  //https://github.com/me-no-dev/AsyncTCP using the latest dev version from @me-no-dev
#include <DNSServer.h>
#include <ESPAsyncWebServer.h>	//https://github.com/me-no-dev/ESPAsyncWebServer using the latest dev version from @me-no-dev
#include <esp_wifi.h>			//Used for mpdu_rx_disable android workaround

// Pre reading on the fundamentals of captive portals https://textslashplain.com/2022/06/24/captive-portals/

const char *ssid = "ETAS SDV.SB";  // FYI The SSID can't have a space in it.
// const char * password = "12345678"; //Atleast 8 chars
const char *password = NULL;  // no password
const char *code = "DEADBEEF"; 
const char *code2 = "0xDEADBEEF"; 

#define MAX_CLIENTS 4	// ESP32 supports up to 10 but I have not tested it yet
#define WIFI_CHANNEL 6	// 2.4ghz channel 6 https://en.wikipedia.org/wiki/List_of_WLAN_channels#2.4_GHz_(802.11b/g/n/ax)

const IPAddress localIP(4, 3, 2, 1);		   // the IP address the web server, Samsung requires the IP to be in public space
const IPAddress gatewayIP(4, 3, 2, 1);		   // IP address of the network should be the same as the local IP for captive portals
const IPAddress subnetMask(255, 255, 255, 0);  // no need to change: https://avinetworks.com/glossary/subnet-mask/

const String localIPURL = "http://4.3.2.1";	 // a string version of the local IP with http, used for redirecting clients to your webpage

volatile long last = 0;
volatile bool turnOff = false;
volatile long offAt = 0;
int led = 13;


const char index_html[] PROGMEM = R"=====(
  <!DOCTYPE html> <html>
    <head>
      <title>ETAS CycurPARTY Mode</title>
      <style>
        body {background: linear-gradient(90deg, #164293, #89037a);}
        h1 {color: white;}
        h2 {color: black;}
        svg {width: 100%; height: auto;} 
      </style>
      <meta name="viewport" content="width=device-width, initial-scale=1.0">
    </head>
    <body>
    <svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 111.69 28.35" width="1920" height="480"><path fill="#fff" d="M90.74 8.93c0-1.48 1.2-2.69 2.67-2.69h12.33c1.91 0 3.1-1.27 3.1-2.84 0-1.57-1.19-2.84-3.1-2.84H93.65c-4.89 0-8.59 3.75-8.59 8.36 0 4.61 3.7 8.36 8.59 8.36h9.68c1.47 0 2.67 1.2 2.67 2.69 0 1.48-1.2 2.69-2.67 2.69H90.99c-1.91 0-3.1 1.27-3.1 2.84 0 1.57 1.19 2.84 3.1 2.84h12.09c4.89 0 8.59-3.75 8.59-8.36s-3.7-8.36-8.59-8.36H93.4c-1.46 0-2.66-1.21-2.66-2.69zM84.6 23.85L72.1 2.21c-.33-.57-.75-.99-1.21-1.27 0 0-.01 0-.01-.01-.33-.19-.68-.31-1.04-.36h-.02c-.07-.01-.14-.01-.21-.02h-.22c-.07 0-.14.01-.21.02h-.02c-.36.04-.71.16-1.04.36 0 0-.01 0-.01.01-.46.27-.88.69-1.21 1.27L54.4 23.86c-.96 1.65-.45 3.32.91 4.11 1.36.78 3.05.39 4.01-1.27L69.49 9.04l10.19 17.65c.96 1.65 2.65 2.05 4.01 1.27 1.36-.79 1.86-2.45.91-4.11zM58.71 3.41c0-1.57-1.19-2.84-3.1-2.84H31.45c-1.91 0-3.1 1.27-3.1 2.84 0 1.57 1.19 2.84 3.1 2.84h9.24v19c0 1.91 1.27 3.1 2.84 3.1s2.84-1.19 2.84-3.1v-19h9.24c1.91 0 3.1-1.28 3.1-2.84zM28.35 14.17C28.35 6.36 21.99 0 14.17 0 6.36 0 0 6.36 0 14.17c0 8.35 6.33 14.17 15.41 14.17h5.09c2.14 0 3.6-1.2 3.6-2.84 0-1.57-1.19-2.84-3.1-2.84h-5.58c-4.61 0-7.95-2.03-9.19-5.37h19.29c1.57 0 2.84-1.27 2.84-2.84v-.28h-.01zm-6.07-2.55H6.07c1.09-3.44 4.31-5.94 8.1-5.94 3.8 0 7.02 2.5 8.11 5.94z"></path></svg>
      <h1>Welcome to ETAS CycurPARTY Mode</h1>
      <hr/>
      Please enter your code to activate Partymode:<br/>
      <form action="/action/login" method="GET">
      <input type="password" name="pw"><br/>
      <input type="submit" value="activate">
      </form>
    </body>
  </html>
)=====";

const char active_html[] PROGMEM = R"=====(
  <!DOCTYPE html> <html>
    <head>
      <title>ETAS CycurPARTY Mode</title>
      <style>
        body {background: linear-gradient(90deg, #164293, #89037a);}
        h1 {color: white;}
        h2 {color: black;}
        svg {width: 100%; height: auto;} 
      </style>
      <meta name="viewport" content="width=device-width, initial-scale=1.0">
    </head>
    <body>
    <svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 111.69 28.35" width="1920" height="480"><path fill="#fff" d="M90.74 8.93c0-1.48 1.2-2.69 2.67-2.69h12.33c1.91 0 3.1-1.27 3.1-2.84 0-1.57-1.19-2.84-3.1-2.84H93.65c-4.89 0-8.59 3.75-8.59 8.36 0 4.61 3.7 8.36 8.59 8.36h9.68c1.47 0 2.67 1.2 2.67 2.69 0 1.48-1.2 2.69-2.67 2.69H90.99c-1.91 0-3.1 1.27-3.1 2.84 0 1.57 1.19 2.84 3.1 2.84h12.09c4.89 0 8.59-3.75 8.59-8.36s-3.7-8.36-8.59-8.36H93.4c-1.46 0-2.66-1.21-2.66-2.69zM84.6 23.85L72.1 2.21c-.33-.57-.75-.99-1.21-1.27 0 0-.01 0-.01-.01-.33-.19-.68-.31-1.04-.36h-.02c-.07-.01-.14-.01-.21-.02h-.22c-.07 0-.14.01-.21.02h-.02c-.36.04-.71.16-1.04.36 0 0-.01 0-.01.01-.46.27-.88.69-1.21 1.27L54.4 23.86c-.96 1.65-.45 3.32.91 4.11 1.36.78 3.05.39 4.01-1.27L69.49 9.04l10.19 17.65c.96 1.65 2.65 2.05 4.01 1.27 1.36-.79 1.86-2.45.91-4.11zM58.71 3.41c0-1.57-1.19-2.84-3.1-2.84H31.45c-1.91 0-3.1 1.27-3.1 2.84 0 1.57 1.19 2.84 3.1 2.84h9.24v19c0 1.91 1.27 3.1 2.84 3.1s2.84-1.19 2.84-3.1v-19h9.24c1.91 0 3.1-1.28 3.1-2.84zM28.35 14.17C28.35 6.36 21.99 0 14.17 0 6.36 0 0 6.36 0 14.17c0 8.35 6.33 14.17 15.41 14.17h5.09c2.14 0 3.6-1.2 3.6-2.84 0-1.57-1.19-2.84-3.1-2.84h-5.58c-4.61 0-7.95-2.03-9.19-5.37h19.29c1.57 0 2.84-1.27 2.84-2.84v-.28h-.01zm-6.07-2.55H6.07c1.09-3.44 4.31-5.94 8.1-5.94 3.8 0 7.02 2.5 8.11 5.94z"></path></svg>
      <h1>Welcome to ETAS CycurPARTY Mode</h1>
      <hr/>
      <form action="/access/ok" method="GET">
      <input type="submit" value="Activate Partymode again">
      </form>
    </body>
  </html>
)=====";

const char index_false_html[] PROGMEM = R"=====(
  <!DOCTYPE html> <html>
    <head>
      <title>ETAS CycurPARTY Mode</title>
      <style>
        body {background: linear-gradient(90deg, #164293, #89037a);}
        h1 {color: white;}
        h2 {color: red; font-weight:bold}
        svg {width: 100%; height: auto;} 
      </style>
      <meta name="viewport" content="width=device-width, initial-scale=1.0">
    </head>
    <body>
    <svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 111.69 28.35" width="1920" height="480"><path fill="#fff" d="M90.74 8.93c0-1.48 1.2-2.69 2.67-2.69h12.33c1.91 0 3.1-1.27 3.1-2.84 0-1.57-1.19-2.84-3.1-2.84H93.65c-4.89 0-8.59 3.75-8.59 8.36 0 4.61 3.7 8.36 8.59 8.36h9.68c1.47 0 2.67 1.2 2.67 2.69 0 1.48-1.2 2.69-2.67 2.69H90.99c-1.91 0-3.1 1.27-3.1 2.84 0 1.57 1.19 2.84 3.1 2.84h12.09c4.89 0 8.59-3.75 8.59-8.36s-3.7-8.36-8.59-8.36H93.4c-1.46 0-2.66-1.21-2.66-2.69zM84.6 23.85L72.1 2.21c-.33-.57-.75-.99-1.21-1.27 0 0-.01 0-.01-.01-.33-.19-.68-.31-1.04-.36h-.02c-.07-.01-.14-.01-.21-.02h-.22c-.07 0-.14.01-.21.02h-.02c-.36.04-.71.16-1.04.36 0 0-.01 0-.01.01-.46.27-.88.69-1.21 1.27L54.4 23.86c-.96 1.65-.45 3.32.91 4.11 1.36.78 3.05.39 4.01-1.27L69.49 9.04l10.19 17.65c.96 1.65 2.65 2.05 4.01 1.27 1.36-.79 1.86-2.45.91-4.11zM58.71 3.41c0-1.57-1.19-2.84-3.1-2.84H31.45c-1.91 0-3.1 1.27-3.1 2.84 0 1.57 1.19 2.84 3.1 2.84h9.24v19c0 1.91 1.27 3.1 2.84 3.1s2.84-1.19 2.84-3.1v-19h9.24c1.91 0 3.1-1.28 3.1-2.84zM28.35 14.17C28.35 6.36 21.99 0 14.17 0 6.36 0 0 6.36 0 14.17c0 8.35 6.33 14.17 15.41 14.17h5.09c2.14 0 3.6-1.2 3.6-2.84 0-1.57-1.19-2.84-3.1-2.84h-5.58c-4.61 0-7.95-2.03-9.19-5.37h19.29c1.57 0 2.84-1.27 2.84-2.84v-.28h-.01zm-6.07-2.55H6.07c1.09-3.44 4.31-5.94 8.1-5.94 3.8 0 7.02 2.5 8.11 5.94z"></path></svg>
      <h1>Welcome to ETAS CycurPARTY Mode</h1>
      <hr/>
<h2>Wrong Code</h2>
      Please enter your code to activate Partymode:<br/>
      <form action="/action/login" method="GET">
      <input type="password" name="pw"><br/>
      <input type="submit" value="activate">
      </form>
    </body>
  </html>
)=====";

DNSServer dnsServer;
AsyncWebServer server(80);

void setUpDNSServer(DNSServer &dnsServer, const IPAddress &localIP) {
// Define the DNS interval in milliseconds between processing DNS requests
#define DNS_INTERVAL 30

	// Set the TTL for DNS response and start the DNS server
	dnsServer.setTTL(3600);
	dnsServer.start(53, "*", localIP);
}

void startSoftAccessPoint(const char *ssid, const char *password, const IPAddress &localIP, const IPAddress &gatewayIP) {
// Define the maximum number of clients that can connect to the server
#define MAX_CLIENTS 4
// Define the WiFi channel to be used (channel 6 in this case)
#define WIFI_CHANNEL 6

	// Set the WiFi mode to access point and station
	WiFi.mode(WIFI_MODE_AP);

	// Define the subnet mask for the WiFi network
	const IPAddress subnetMask(255, 255, 255, 0);

	// Configure the soft access point with a specific IP and subnet mask
	WiFi.softAPConfig(localIP, gatewayIP, subnetMask);

	// Start the soft access point with the given ssid, password, channel, max number of clients
	WiFi.softAP(ssid, password, WIFI_CHANNEL, 0, MAX_CLIENTS);

	// Disable AMPDU RX on the ESP32 WiFi to fix a bug on Android
	esp_wifi_stop();
	esp_wifi_deinit();
	wifi_init_config_t my_config = WIFI_INIT_CONFIG_DEFAULT();
	my_config.ampdu_rx_enable = false;
	esp_wifi_init(&my_config);
	esp_wifi_start();
	vTaskDelay(100 / portTICK_PERIOD_MS);  // Add a small delay
}

void setUpWebserver(AsyncWebServer &server, const IPAddress &localIP) {
	//======================== Webserver ========================
	// WARNING IOS (and maybe macos) WILL NOT POP UP IF IT CONTAINS THE WORD "Success" https://www.esp8266.com/viewtopic.php?f=34&t=4398
	// SAFARI (IOS) IS STUPID, G-ZIPPED FILES CAN'T END IN .GZ https://github.com/homieiot/homie-esp8266/issues/476 this is fixed by the webserver serve static function.
	// SAFARI (IOS) there is a 128KB limit to the size of the HTML. The HTML can reference external resources/images that bring the total over 128KB
	// SAFARI (IOS) popup browser has some severe limitations (javascript disabled, cookies disabled)

	// Required
	server.on("/connecttest.txt", [](AsyncWebServerRequest *request) { request->redirect("http://logout.net"); });	// windows 11 captive portal workaround
	server.on("/wpad.dat", [](AsyncWebServerRequest *request) { request->send(404); });								// Honestly don't understand what this is but a 404 stops win 10 keep calling this repeatedly and panicking the esp32 :)

	// Background responses: Probably not all are Required, but some are. Others might speed things up?
	// A Tier (commonly used by modern systems)
	server.on("/generate_204", [](AsyncWebServerRequest *request) { request->redirect(localIPURL); });		   // android captive portal redirect
	server.on("/redirect", [](AsyncWebServerRequest *request) { request->redirect(localIPURL); });			   // microsoft redirect
	server.on("/hotspot-detect.html", [](AsyncWebServerRequest *request) { request->redirect(localIPURL); });  // apple call home
	server.on("/canonical.html", [](AsyncWebServerRequest *request) { request->redirect(localIPURL); });	   // firefox captive portal call home
	server.on("/success.txt", [](AsyncWebServerRequest *request) { request->send(200); });					   // firefox captive portal call home
	server.on("/ncsi.txt", [](AsyncWebServerRequest *request) { request->redirect(localIPURL); });			   // windows call home

	// B Tier (uncommon)
	//  server.on("/chrome-variations/seed",[](AsyncWebServerRequest *request){request->send(200);}); //chrome captive portal call home
	//  server.on("/service/update2/json",[](AsyncWebServerRequest *request){request->send(200);}); //firefox?
	//  server.on("/chat",[](AsyncWebServerRequest *request){request->send(404);}); //No stop asking Whatsapp, there is no internet connection
	//  server.on("/startpage",[](AsyncWebServerRequest *request){request->redirect(localIPURL);});

	// return 404 to webpage icon
	server.on("/favicon.ico", [](AsyncWebServerRequest *request) { request->send(404); });	// webpage icon

	// Serve Basic HTML Page
	server.on("/", HTTP_ANY, [](AsyncWebServerRequest *request) {
		AsyncWebServerResponse *response = request->beginResponse(200, "text/html", index_html);
		response->addHeader("Cache-Control", "public,max-age=31536000");  // save this file to cache for 1 year (unless you refresh)
		request->send(response);
		Serial.println("Served Basic HTML Page");
	});

// Serve Basic HTML Page
	server.on("/action/login", HTTP_ANY, [](AsyncWebServerRequest *request) {
    if (request->hasParam("pw")) {
      
		  //Serial.println("Served Login Check");
      if(request->getParam("pw")->value() == code || request->getParam("pw")->value() == code2) {
      //  Serial.println("Correct Code");
        request->redirect("/access/ok");
      } else {
      //  Serial.println("Wrong Code");
        request->redirect("/access/no");
      }
    } else {
  request->redirect("/");
    }



	});

  // Serve Failed Code HTML Page
	server.on("/access/no", HTTP_ANY, [](AsyncWebServerRequest *request) {
		AsyncWebServerResponse *response = request->beginResponse(200, "text/html", index_false_html);
		response->addHeader("Cache-Control", "public,max-age=31536000");  // save this file to cache for 1 year (unless you refresh)
		request->send(response);
		Serial.println("Served False HTML Page");
	});

// Serve Working Code HTML Page
	server.on("/access/ok", HTTP_ANY, [](AsyncWebServerRequest *request) {
    // Turn on the Party!!!!
    turnOff = true;
    offAt = millis() + 5000; //save a variable of now + 5 seconds
    digitalWrite(led, HIGH); //turn on

		AsyncWebServerResponse *response = request->beginResponse(200, "text/html", active_html);
		response->addHeader("Cache-Control", "public,max-age=31536000");  // save this file to cache for 1 year (unless you refresh)
		request->send(response);
		Serial.println("Served OK HTML Page");
	});


	// the catch all
	server.onNotFound([](AsyncWebServerRequest *request) {
		request->redirect(localIPURL);
		Serial.print("onnotfound ");
		Serial.print(request->host());	// This gives some insight into whatever was being requested on the serial monitor
		Serial.print(" ");
		Serial.print(request->url());
		Serial.print(" sent redirect to " + localIPURL + "\n");
	});
}

void setup() {
	// Set the transmit buffer size for the Serial object and start it with a baud rate of 115200.
	Serial.setTxBufferSize(1024);
	Serial.begin(115200);

	// Wait for the Serial object to become available.
	while (!Serial)
		;

    pinMode(led, OUTPUT);

	// Print a welcome message to the Serial port.
	Serial.println("\n\nCaptive Test, V0.5.0 compiled " __DATE__ " " __TIME__ " by CD_FER");  //__DATE__ is provided by the platformio ide
	Serial.printf("%s-%d\n\r", ESP.getChipModel(), ESP.getChipRevision());

	startSoftAccessPoint(ssid, password, localIP, gatewayIP);

	setUpDNSServer(dnsServer, localIP);

	setUpWebserver(server, localIP);
	server.begin();

	Serial.print("\n");
	Serial.print("Startup Time:");	// should be somewhere between 270-350 for Generic ESP32 (D0WDQ6 chip, can have a higher startup time on first boot)
	Serial.println(millis());
	Serial.print("\n");
}



void loop() {
	dnsServer.processNextRequest();	 // I call this atleast every 10ms in my other projects (can be higher but I haven't tested it for stability)
    if(turnOff)
  {
   if(millis() >= offAt)
    {
       digitalWrite(led, LOW); //turn off led
    } 
  }
	delay(DNS_INTERVAL);			 // seems to help with stability, if you are doing other things in the loop this may not be needed
}