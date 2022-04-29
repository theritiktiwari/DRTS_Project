#include <ESP8266WiFi.h>
#include <WiFiClientSecure.h>
#include <UniversalTelegramBot.h>
#include <NTPClient.h>
#include <WiFiUdp.h>

// Wifi network station credentials
#define WIFI_SSID "network_name"
#define WIFI_PASSWORD "network_pass"

// Telegram BOT Token (Get from Botfather)
#define BOT_TOKEN "token"

const unsigned long BOT_MTBS = 1000; // mean time between scan messages

const long utcOffsetInSeconds = 3600;

X509List cert(TELEGRAM_CERTIFICATE_ROOT);
WiFiClientSecure secured_client;
UniversalTelegramBot bot(BOT_TOKEN, secured_client);

// Define NTP Client to get time
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org", utcOffsetInSeconds);

unsigned long bot_lasttime; // last time messages' scan has been done

const int ledPin = D1;
const int ldrPin = A0;
const int buzzer = D5;

int ledStatus = 0;
int buzz = 0;

void handleNewMessages(int numNewMessages, int ldrStatus)
{
  Serial.print("handleNewMessages ");
  Serial.println(numNewMessages);

  for (int i = 0; i < numNewMessages; i++)
  {
    String chat_id = bot.messages[i].chat_id;
    String text = bot.messages[i].text;

    String from_name = bot.messages[i].from_name;
    if (from_name == "")
      from_name = "Guest";

    if (text == "/ledon")
    {
      if (ldrStatus <= 500)
      {
        digitalWrite(ledPin, HIGH); // turn the LED on (HIGH is the voltage level)
        ledStatus = 1;
        bot.sendMessage(chat_id, "Led is ON", "");
      }
      else
      {
        digitalWrite(ledPin, LOW);
        ledStatus = 0;
        bot.sendMessage(chat_id, "It's not dark so Led is OFF", "");
      }
    }
    
    else if (text == "/ledoff")
    {
      ledStatus = 0;
      digitalWrite(ledPin, LOW); // turn the LED off (LOW is the voltage level)
      bot.sendMessage(chat_id, "Led is OFF", "");
    }

    else if (text == "/status")
    {
      if (ledStatus)
      {
        bot.sendMessage(chat_id, "Led is ON", "");
      }
      else
      {
        bot.sendMessage(chat_id, "Led is OFF", "");
      }
    }
    
    else if (text == "/start")
    {
      String welcome = "Welcome " + from_name + "! to the DRTS Project.\n";
      welcome += "This is Night Lamp with Morning Alarm.\n\n";
      welcome += "/ledon : to switch the Led ON\n";
      welcome += "/ledoff : to switch the Led OFF\n";
      welcome += "/status : Returns current status of LED\n";
      bot.sendMessage(chat_id, welcome, "Markdown");
    }

    else
    {
      bot.sendMessage(chat_id, "Please Choose Correct Command", "");
      
    }
  }
}

void alarm(int ldrStatus)
{
  if (buzz == 0 && ledStatus == 0 && ldrStatus > 500)
  {
    Serial.print(timeClient.getHours());
    Serial.print(":");
    Serial.print(timeClient.getMinutes());
    Serial.print(":");
    Serial.println(timeClient.getSeconds());
    Serial.println(timeClient.getFormattedTime());

    int h = timeClient.getHours();
    int m = timeClient.getMinutes();
    int s = timeClient.getSeconds();
  
    delay(1000);
  
    digitalWrite(buzzer, HIGH);
    Serial.println("Alarm");
    delay(2000);
    digitalWrite(buzzer, LOW);
    buzz = 1;
  }
}

void setup()
{
  Serial.begin(115200);
  Serial.println();

  pinMode(ledPin, OUTPUT);
  pinMode(buzzer, OUTPUT);
  pinMode(ldrPin, INPUT);

  // attempt to connect to Wifi network:
  configTime(0, 0, "pool.ntp.org");      // get UTC time via NTP
  secured_client.setTrustAnchors(&cert); // Add root certificate for api.telegram.org
  Serial.print("Connecting to Wifi SSID ");
  Serial.print(WIFI_SSID);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  while (WiFi.status() != WL_CONNECTED)
  {
    Serial.print(".");
    delay(500);
  }
  Serial.print("\nWiFi connected. \nIP address: ");
  Serial.println(WiFi.localIP());

  // Check NTP/Time, usually it is instantaneous and you can delete the code below.
  Serial.print("Retrieving time: ");
  time_t now = time(nullptr);
  while (now < 24 * 3600)
  {
    Serial.print(".");
    delay(100);
    now = time(nullptr);
  }
  Serial.println(now);

  timeClient.begin();
}

void loop()
{
  timeClient.update();
  int ldrStatus = analogRead(ldrPin);

  if (millis() - bot_lasttime > BOT_MTBS)
  {
    int numNewMessages = bot.getUpdates(bot.last_message_received + 1);

    while (numNewMessages)
    {
      Serial.println("got response");
      handleNewMessages(numNewMessages, ldrStatus);
      numNewMessages = bot.getUpdates(bot.last_message_received + 1);
    }

    bot_lasttime = millis();
  }
  else
  {
    if (ldrStatus <= 500)
    {
      digitalWrite(ledPin, HIGH);
      Serial.print(ldrStatus);
      Serial.println(" LDR is DARK, LED is ON");
      buzz = 0;
      ledStatus = 1;
    }
    else
    {
      digitalWrite(ledPin, LOW);
      ledStatus = 0;
      alarm(ldrStatus);
    }
  }
}
