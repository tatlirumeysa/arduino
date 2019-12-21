
#include <SoftwareSerial.h>
#include <dht11.h> // dht11 kütüphanesini ekliyoruz.
#include <Wire.h>
#include <RTClib.h>
#include <LiquidCrystal_I2C.h>

//************************************//
LiquidCrystal_I2C lcd(39, 2, 1, 0, 4, 5, 6, 7);
RTC_DS1307 RTC;

String agAdi = "yourNetworkName";                 //Ağımızın adını buraya yazıyoruz.
String agSifresi = "YourNetworkPaswd";           //Ağımızın şifresini buraya yazıyoruz.
unsigned long eskiZaman = 0;
unsigned long yeniZaman;
int rxPin = 10;                                               //ESP8266 RX pini
int txPin = 11;                                               //ESP8266 TX pini
int dht11Pin = 2;
int P1 = 14; // SAAT
int P2 = 15; // DAKIKA

String ip = "127.0.0.1";                                //Thingspeak ip adresi
float sicaklik, nem;
int SAAT;
int DAKIKA;
int YIL = 2019;
int AY = 1;
int GUN = 1;
long GONDERME_SURESI = 1000;
dht11 DHT11;

SoftwareSerial esp(rxPin, txPin);                             //Seri haberleşme pin ayarlarını yapıyoruz.

void setup() {

  Serial.begin(9600);  //Seri port ile haberleşmemizi başlatıyoruz.


  lcd.begin(16, 2);              // LCD KUTUPHANE AYARI
  lcd.setBacklightPin(3, POSITIVE);
  lcd.setBacklight(HIGH);
  lcd.clear();

  pinMode(P1, INPUT_PULLUP); 
  pinMode(P2, INPUT_PULLUP);

  Wire.begin();
  RTC.begin();

  if (! RTC.isrunning()) {
    Serial.println("RTC is NOT running!");
    // Set the date and time at compile time
    RTC.adjust(DateTime(__DATE__, __TIME__));
  }
  // RTC.adjust(DateTime(__DATE__, __TIME__)); //removing "//" to adjust the time
  // The default display shows the date and time

  lcd.setCursor(0, 0);
  lcd.print( "ESP AYARLANIYOR ");

  Serial.println("Started");
  esp.begin(115200);                                          //ESP8266 ile seri haberleşmeyi başlatıyoruz.
  esp.println("AT");                                          //AT komutu ile modül kontrolünü yapıyoruz.
  Serial.println("AT Yollandı");
  while (!esp.find("OK")) {                                   //Modül hazır olana kadar bekliyoruz.
    esp.println("AT");
    Serial.println("ESP8266 Bulunamadı.");
    lcd.setCursor(0, 0);
    lcd.print( "ESP BEKLENIYOR ");
  }
    lcd.setCursor(0, 0);
    lcd.print( "ILETISIM HAZIR ");
  Serial.println("OK Komutu Alındı");
  esp.println("AT+CWMODE=1");                                 //ESP8266 modülünü client olarak ayarlıyoruz.
  while (!esp.find("OK")) {                                   //Ayar yapılana kadar bekliyoruz.
    esp.println("AT+CWMODE=1");
    Serial.println("Ayar Yapılıyor....");
    lcd.print( "AYAR YAPILIYOR ");
  }
  Serial.println("Client olarak ayarlandı");
  Serial.println("Aga Baglaniliyor...");
  lcd.print( "AGA BAGLANILIYOR..");
  esp.println("AT+CWJAP=\"" + agAdi + "\",\"" + agSifresi + "\""); //Ağımıza bağlanıyoruz.
  while (!esp.find("OK"));                                    //Ağa bağlanana kadar bekliyoruz.
  Serial.println("Aga Baglandi.");
  lcd.setCursor(0, 0);
  lcd.print( "AGA BAGLANDI    ");
  delay(1000);
}
void loop() {
  esp.println("AT+CIPSTART=\"TCP\",\"" + ip + "\",80");       //Thingspeak'e bağlanıyoruz.
  if (esp.find("Error")) {                                    //Bağlantı hatası kontrolü yapıyoruz.
    Serial.println("AT+CIPSTART Error");
  }
  DHT11.read(dht11Pin);
  sicaklik = (float)DHT11.temperature;
  nem = (float)DHT11.humidity;

  yeniZaman = millis();

  lcd.clear();

  DateTime now = RTC.now();

  lcd.setCursor(0, 0);

  if (now.hour() <= 9)
  {
    lcd.print("0");
  }
  lcd.print(now.hour(), DEC);
  SAAT = now.hour();
  lcd.print(":");
  if (now.minute() <= 9)
  {
    lcd.print("0");
  }
  lcd.print(now.minute(), DEC);
  DAKIKA = now.minute();

  lcd.print("           ");
  lcd.setCursor(0, 1);
  lcd.print( sicaklik, 0);
  lcd.print("c  %");
  lcd.print( nem, 0);
  lcd.print(" ");

  if (digitalRead(P1) == LOW)
  {

    SAAT = SAAT + 1;
    if (SAAT == 24)
    {
      SAAT = 0;
    }

    RTC.adjust(DateTime(YIL, AY, GUN, SAAT, DAKIKA, 0));

    delay(500);
  }

  if (digitalRead(P2) == LOW)
  {
    DAKIKA = DAKIKA + 1;
    if (DAKIKA == 60)
    {
      DAKIKA = 0;
    }

    RTC.adjust(DateTime(YIL, AY, GUN, SAAT, DAKIKA, 0));

    delay(500);
  }

  /* bir önceki turdan itibaren 10 saniye geçmiş mi
    yani yeniZaman ile eskiZaman farkı 1000den büyük mü */
  if (yeniZaman - eskiZaman > GONDERME_SURESI) {
    String veri = "GET https://api.thingspeak.com/update?api_key=1OW5MJJOGVLDNKSE";   //Thingspeak komutu. Key kısmına kendi api keyimizi yazıyoruz.                                   //Göndereceğimiz sıcaklık değişkeni
    veri += "&field1=";
    veri += String(sicaklik);
    veri += "&field2=";
    veri += String(nem);                                        //Göndereceğimiz nem değişkeni
    veri += "\r\n\r\n";
    esp.print("AT+CIPSEND=");                                   //ESP'ye göndereceğimiz veri uzunluğunu veriyoruz.
    esp.println(veri.length() + 2);
    delay(500);
    if (esp.find(">")) {                                        //ESP8266 hazır olduğunda içindeki komutlar çalışıyor.
      esp.print(veri);                                          //Veriyi gönderiyoruz.
      Serial.println(veri);
      Serial.println("Veri gonderildi.");
      delay(100);
    }
    Serial.println("Baglantı Kapatildi.");
    esp.println("AT+CIPCLOSE");                                //Bağlantıyı kapatıyoruz
    /* Eski zaman değeri yeni zaman değeri ile güncelleniyor */
    eskiZaman = yeniZaman;
  }


}
