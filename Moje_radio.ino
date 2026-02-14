// W Arduino IDE musimy zainstalować biblioteki:
// 1. ESP32-audioI2S
// 2. Adafruit GFX Library
// 3. Adafruit ST7735 and ST7789 - bibliotekę dobieramy do naszego wyświetlacza (w tym przykładzie używam wyświetlacza ST7789); np. dla ILI9341 instalujemy bibliotekę Adafruit ILI9341, itd)
#include <WiFi.h>
#include "Audio.h" // (https://github.com/schreibfaul1/ESP32-audioI2S)
#include "Adafruit_GFX.h"
#include <Adafruit_ST7789.h> // dołączamy bibliotekę obsługującą nasz wyświetlacz (np. dla ILI9341 będzie to #include <Adafruit_ILI9341.h>, itd)

// Tutaj definiujemy piny, których użyliśmy do podłączenia naszego wyświetlacza SPI
#define TFT_DC 14
#define TFT_RST 18
#define TFT_CS 10
#define TFT_BL 21
#define TFT_MOSI 11 // <- WAŻNE: sprzętowe SPI
#define TFT_SCLK 12 // <- WAŻNE: sprzętowe SPI 

// Tutaj definiujemy piny, których użyliśmy do podłączenia PCM5102A
#define I2S_DOUT  5    
#define I2S_BCLK  7    
#define I2S_LRC   6 

// Definicja niezbędnych obiektów
SPIClass spi(HSPI); // Interfejs SPI
Adafruit_ST7789 tft = Adafruit_ST7789(&spi, TFT_CS, TFT_DC, TFT_RST); // Wyświetlacz SPI
Audio audio; // Odtwarzacz audio


char stacja_nazwa[] = "RMF FM"; // Nazwa stacji
char stacja_url[] = "https://rs101-krk-cyfronet.rmfstream.pl/RMFFM48"; // Adres stacji

// Opcjonalna funkcja wyświetlająca komunikaty odtwarzacza audio w Serial Monitorze oraz nazwę streamu na wyświetlaczu
void my_audio_info(Audio::msg_t m) {
  Serial.printf("%s: %s\n", m.s, m.msg);
  if (m.e == Audio::evt_streamtitle) {
    tft.setCursor(0, 140);
    tft.setTextColor(ST77XX_CYAN, ST77XX_BLACK);
    tft.print(m.msg);
  }
}

// Funkcja setup
void setup() {
  pinMode(TFT_BL, OUTPUT); // Ustawienie pinu odpowiedzialnego za podświetlenie wyświetlacza jako wyjście
  digitalWrite(TFT_BL, HIGH); // Włączenie podświetlenia wyświetlacza
  
  Serial.begin(115200); // Inicjalizacja monitora portu szeregowego, tj. Serial Monitor
  delay(2000); // Czekamy 2 sekundy na ustabilizowanie portu szeregowego
  
  WiFi.begin("nazwa_sieci_wifi", "haslo_sieci_wifi"); // Podłączenie do sieci WiFi
  
  spi.begin(TFT_SCLK, -1, TFT_MOSI, TFT_CS); // Inicjalizacja interfejsu SPI
  
  tft.init(240, 320); // Inicjalizacja wyświetlacza ST7789 ( dla ILI9341 zamiast tego będzie tft.begin(); )
  tft.setRotation(1); // Dopasowanie obrotu obrazu ( dla ILI9341 zamiast tego będzie tft.setRotation(3); )
  tft.setTextWrap(true); // Tekst dłuższy niż szerokość ekranu będzie zawijany do kolejnej linii
  tft.fillScreen(ST77XX_BLACK); // Ustawiamy kolor tła wyświetlacza
  tft.setTextSize(3); // Ustawiamy rozmiar czcionki wyświetlacza na 3
  tft.setTextColor(ST77XX_WHITE); // Ustawiamy kolor czcionki wyświetlacza
  tft.setCursor(0,0); // Ustawiamy pozycję (x, y) tekstu, na której będzie wyswietlony tekst
  tft.print("RADIO INTERNETOWE"); // Wyswietlamy napis "RADIO INTERNETOWE" na pozycji (0, 0)
  tft.setTextSize(2); // Ustawiamy rozmiar czcionki wyświetlacza na 2
  tft.setCursor(0,50); tft.println("Stacja:"); // Wyświetlamy napis "Stacja:" na pozycji (0, 50)
  tft.setTextColor(ST77XX_CYAN, ST77XX_BLACK); tft.setCursor(0,70); tft.print(stacja_nazwa); // Zmieniamy kolor czcionki na cyjanowy i wyświetlamy zdefiniowaną wcześniej nazwę stacji na pozycji (0, 70)
  tft.setTextColor(ST77XX_WHITE); tft.setCursor(0, 120); tft.print("Stream:"); // Zmieniamy kolor czcionki na biały i wyświetlamy napis "Stream:" na pozycji (0, 120)
 
  // Czekamy na połączenie z siecią WiFi
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print("*");
    delay(100);
  }
  Serial.println("\nWiFi Połączone :)");

  // Ustawiamy konfigurację odtwarzacza Audio
  audio.setPinout(I2S_BCLK, I2S_LRC, I2S_DOUT); // Informujemy odtwarzacz o tym, jakich pinów ma używać
  Audio::audio_info_callback = my_audio_info; // Opcjonalnie - funkcja wyświetla szczegóły połączenie na Serial Monitorze i tytułstreamu na wyświetlaczu
  audio.setVolume(3); // Ustawiamy głośność dźwięku (standardowy zakres 0...21)
  audio.connecttohost(stacja_url); // podłączamy się do zdefiniowanej wcześniej stacji, po czym powinniśmy uszłyszeć dżwięk (szczegóły połączenie możemy obserwować na Serial Monitorze)
}

void loop() {
  audio.loop(); // główna pętla odtwarzacza
  delay(1); // opóżnienie 1ms zapobiegające ew. błędom Watchdog Error
}
