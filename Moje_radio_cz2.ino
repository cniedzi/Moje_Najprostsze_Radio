// W Arduino IDE musimy zainstalować biblioteki:
// 1. ESP32-audioI2S
// 2. Adafruit GFX Library
// 3. Adafruit ST7735 and ST7789 - bibliotekę dobieramy do naszego wyświetlacza (w tym przykładzie używam wyświetlacza ST7789); np. dla ILI9341 instalujemy bibliotekę Adafruit ILI9341, itd)
#include <WiFi.h>
#include "Audio.h" // (https://github.com/schreibfaul1/ESP32-audioI2S)
#include "Adafruit_GFX.h"
#include <Adafruit_ST7789.h> // dołączamy bibliotekę obsługującą nasz wyświetlacz (np. dla ILI9341 będzie to #include <Adafruit_ILI9341.h>, itd)

// ------------------ E N K O D E R --------------------//
#include "Rotary.h"  // https://github.com/buxtronix/arduino/tree/master/libraries/Rotary) by Ben Buxton (http://www.buxtronix.net/2011/10/rotary-encoders-done-properly.html;
// ------------------ E N K O D E R --------------------//

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

// ------------------ E N K O D E R --------------------//
// Tutaj definiujemy piny enkoderów; jeżeli okaże się, że enkodery działają odwrotnie, to zmieniamy definicję pinów S1 z S2
#define buttonEncoderL 4 // przycisk lewego enkodera
#define S1_EncoderL 1 // S1 lewego enkodera
#define S2_EncoderL 2 // S2 lewego enkodera
// ------------------ E N K O D E R --------------------//

// ----------------- G Ł O Ś N O Ś Ć -------------------//
// Definicje zakresu głośności
#define VOLUME_MIN 0
#define VOLUME_MAX 21
// ----------------- G Ł O Ś N O Ś Ć -------------------//

// Definicja niezbędnych obiektów
SPIClass spi(HSPI); // Interfejs SPI
Adafruit_ST7789 tft = Adafruit_ST7789(&spi, TFT_CS, TFT_DC, TFT_RST); // Wyświetlacz SPI
Audio audio; // Odtwarzacz audio

// ------------------ E N K O D E R --------------------//
Rotary EncL = Rotary(S1_EncoderL, S2_EncoderL); // Lewy enkoder
// ------------------ E N K O D E R --------------------//

char stacja_nazwa[] = "Italo 4 you"; // Nazwa stacji
char stacja_url[] = "http://s0.radiohost.pl:8018/stream"; // Adres stacji

// ----------------- G Ł O Ś N O Ś Ć -------------------//
int g_Volume = 5; // zmienna globalna przechowująca aktualną głośność radia
// ----------------- G Ł O Ś N O Ś Ć -------------------//

// Opcjonalna funkcja wyświetlająca komunikaty odtwarzacza audio w Serial Monitorze oraz nazwę streamu na wyświetlaczu
void my_audio_info(Audio::msg_t m) {
  Serial.printf("%s: %s\n", m.s, m.msg);
  if (m.e == Audio::evt_streamtitle) {
    tft.setTextColor(ST77XX_CYAN, ST77XX_BLACK);
    tft.setCursor(0, 140);
    tft.print("                                                                                                                               "); // Czyścimy poprzedni tytuł streamu
    tft.setCursor(0, 140);
    tft.print(m.msg);
  }
}

// Funkcja setup
void setup() {
  pinMode(TFT_BL, OUTPUT); // Ustawienie pinu odpowiedzialnego za podświetlenie wyświetlacza jako wyjście
  digitalWrite(TFT_BL, HIGH); // Włączenie podświetlenia wyświetlacza

  // Ustawienie pinów enkodera jako wejść
  pinMode(, INPUT_PULLUP);
  pinMode(S1_EncoderL, INPUT_PULLUP);
  pinMode(S2_EncoderL, INPUT_PULLUP);
  
  Serial.begin(115200); // Inicjalizacja monitora portu szeregowego, tj. Serial Monitor
  delay(2000); // Czekamy 2 sekundy na ustabilizowanie portu szeregowego
  
  WiFi.begin("nazwa_sieci_wifi", "haslo_sieci_wifi"); // Podłączenie do sieci WiFi -> WAŻNE: trzeba wpisać swoje parametry sieci WiFi
  
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

  // ----------------- G Ł O Ś N O Ś Ć -------------------//
  tft.setTextColor(ST77XX_GREEN); tft.setCursor(0, 225); tft.print("Volume:"); // Zmieniamy kolor czcionki na zielony i wyświetlamy napis "Volume:" na pozycji (0, 225)
  tft.setTextColor(ST77XX_WHITE); tft.setCursor(96, 225); tft.printf("%02d", g_Volume); // Zmieniamy kolor czcionki na biały i wyświetlamy aktualną wartość głośności na pozycji (96, 225);
                                                                                        // format wyświetlania "%02d" powoduje, że zawsze wyświetlana jest liczba 2-cyfrowa, np. 01
  // ----------------- G Ł O Ś N O Ś Ć -------------------//
 
  // Czekamy na połączenie z siecią WiFi
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print("*");
    delay(100);
  }
  Serial.println("\nWiFi Połączone :)");

  // Ustawiamy konfigurację odtwarzacza Audio
  audio.setPinout(I2S_BCLK, I2S_LRC, I2S_DOUT); // Informujemy odtwarzacz o tym, jakich pinów ma używać
  Audio::audio_info_callback = my_audio_info; // Opcjonalnie - funkcja wyświetla szczegóły połączenie na Serial Monitorze i tytułstreamu na wyświetlaczu
  
  // ----------------- G Ł O Ś N O Ś Ć -------------------//
  audio.setVolume(g_Volume); // Ustawiamy głośność dźwięku (standardowy zakres 0...21)
  // ----------------- G Ł O Ś N O Ś Ć -------------------//
  
  audio.connecttohost(stacja_url); // podłączamy się do zdefiniowanej wcześniej stacji, po czym powinniśmy uszłyszeć dżwięk (szczegóły połączenie możemy obserwować na Serial Monitorze)
}

void loop() {

  // --------- E N K O D E R + G Ł O Ś N O Ś Ć -----------//
  unsigned char result = EncL.process(); // sprawdzamy stan lewego enkodera

  if (result == DIR_CW) { // Jeżeli obrót zgodnie z ruchem wskazówek zegara, to...
    if (g_Volume < VOLUME_MAX) g_Volume++; // jeżeli aktualna głośność jest mniejsza niż wartość maksymalna, zwiększ Volume o 1
    audio.setVolume(g_Volume); // ustawiamy wartość g_Volume jako aktualną głośność radia
    tft.setTextColor(ST77XX_WHITE, ST77XX_BLACK); tft.setCursor(96, 225); tft.printf("%02d", g_Volume); // Zmieniamy kolor czcionki na biały i wyświetlamy aktualną wartość głośności na pozycji (96, 225);
                                                                                                      // WAŻNE: ustawiliśmy też czarny kolor tła czcionki, dzięki czemu poprzednia wartość będzie wymazywana,
                                                                                                      // a format wyświetlania "%02d" powoduje, że zawsze wyświetlana jest liczba 2-cyfrowa, np. 01,
                                                                                                      // dzięki czemu przy przejściu z liczby 10 na 9 nie pozostanie stara nadmiarowa cyfra 0.
    Serial.printf("Volume: %d\n", g_Volume); // wyświetlamy aktualną głośność na Serial Monitorze
  }
  else if (result == DIR_CCW) { // Jeżeli obrót przeciwnie do ruchu wskazówek zegara, to...
    if (g_Volume > VOLUME_MIN) g_Volume--; // jeżeli aktualna głośność jest większa niż wartość minimalna, zmniejsz Volume o 1
    audio.setVolume(g_Volume); // ustawiamy wartość g_Volume jako aktualną głośność radia
    tft.setTextColor(ST77XX_WHITE, ST77XX_BLACK); tft.setCursor(96, 225); tft.printf("%02d", g_Volume); // analogicznie dla obrotu enkodera DIR_CW
    Serial.printf("Volume: %d\n", g_Volume); // wyświetlamy aktualną głośność na Serial Monitorze
  }
  // --------- E N K O D E R + G Ł O Ś N O Ś Ć -----------//

  audio.loop(); // główna pętla odtwarzacza
  delay(1); // opóżnienie 1ms zapobiegające ew. błędom Watchdog Error
}
