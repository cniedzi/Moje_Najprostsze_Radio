// Copyright (c) 2026 Cezary Niedziński
// Licensed under the Polyform Noncommercial License 1.0.0.

// W Arduino IDE musimy zainstalować biblioteki:
// 1. ESP32-audioI2S
// 2. Adafruit GFX Library
// 3. LovyanGFX
// 4. FFT by Robin Scheibler (http://www.robinscheibler.org)

#include <WiFi.h>
#include <Preferences.h>
#include "Audio.h" // (https://github.com/schreibfaul1/ESP32-audioI2S)


// ---------------- L O V Y A N G F X ------------------//
#include <Lovyan_Config.h>
#include "Lato_Bold_22.h"
// ---------------- L O V Y A N G F X ------------------//

// --------------------- L O G O -----------------------//
#include "Logo.h"
// --------------------- L O G O -----------------------//

// ------------------ E N K O D E R --------------------//
#include "Rotary.h"  // https://github.com/buxtronix/arduino/tree/master/libraries/Rotary) by Ben Buxton (http://www.buxtronix.net/2011/10/rotary-encoders-done-properly.html;
// ------------------ E N K O D E R --------------------//

// ---------------- W Y G A S Z A C Z -------------------//
#include "Images.h"
// ---------------- W Y G A S Z A C Z -------------------//

// --------- A N A L I Z A T O R   W I D M A ------------//
#include "fft.h" // FFT by Robin Scheibler (http://www.robinscheibler.org)
// --------- A N A L I Z A T O R   W I D M A ------------//

// Tutaj definiujemy piny, których użyliśmy do podłączenia PCM5102A
#define I2S_DOUT  5    
#define I2S_BCLK  7    
#define I2S_LRC   6 


// ------------------ E N K O D E R --------------------//
// Tutaj definiujemy piny enkoderów; jeżeli okaże się, że enkodery działają odwrotnie, to zmieniamy definicję pinów S1 z S2
#define buttonEncoderL 4 // przycisk lewego enkodera
#define S1_EncoderL 1 // S1 lewego enkodera
#define S2_EncoderL 2 // S2 lewego enkodera
#define buttonEncoderR 15 // przycisk prawego enkodera
#define S1_EncoderR 16 // S1 prawego enkodera
#define S2_EncoderR 17 // S2 prawego enkodera
// ------------------ E N K O D E R --------------------//

// ----------------- G Ł O Ś N O Ś Ć -------------------//
// Definicje zakresu głośności
#define VOLUME_MIN 0
#define VOLUME_MAX 21
#define VOLUME_KOLOR 0xf492 // https://rgbcolorpicker.com/565 <- tutaj można sprawdzać kody doowolnych kolorów RGB565 (3 pole tekstowe)
// ----------------- G Ł O Ś N O Ś Ć -------------------//

// ----------------------- V U -------------------------//
#define VU_POS_X 160 // Współrzędna X lewego dolnego rogu wskaźnika VU
#define VU_POS_Y 236 // Współrzędna Y lewego dolnego rogu wskaźnika VU
// ----------------------- V U -------------------------//


// --------------------- W i F i -----------------------//
#define KOLOR_WIFI 0xBDD7 // Definicja koloru do wyświetlania parametrów WiFi
#define WIFI_POS_Y 212 // Definicja współrzędnej Y (w pionie) parametrów WiFi
#define SSID_POS_X 120 // Definicja współrzędnej X (w poziomie) nazwy sieci WiFi
// --------------------- W i F i -----------------------//



// ---------------- W Y G A S Z A C Z -------------------//
#define WYGASZACZ_TIMEOUT 60000 // Definicja czasu w [ms], po którym aktywuje się wygaszacz
#define WYGASZACZ_IMAGE_DURATION 10000 // Definicja czasu w [ms], po którym wyswietlony zostanie kolejny obraz wygaszacza ekranu
// ---------------- W Y G A S Z A C Z -------------------//




// -------- T R Y B Y   P R A C Y   R A D I A -----------//
#define MODE_MAIN 0
#define MODE_SCREENSAVER 1
#define MODE_SPECTRUM_ANALYZER_1 2
#define MODE_SPECTRUM_ANALYZER_2 3
// -------- T R Y B Y   P R A C Y   R A D I A -----------//


// --------- A N A L I Z A T O R   W I D M A ------------//
#define SIZE_OF_FFT 512
#define DISPLAY_ANALYZER_PERIOD 50 //ms
#define SMOOTHING_FACTOR_S16 0.25f // im mniejsze, tym bardziej wygładzone (0 < SMOOTHING_FACTOR < 1)
// --------- A N A L I Z A T O R   W I D M A ------------//



// Definicja niezbędnych obiektów
SPIClass spi(HSPI); // Interfejs SPI
LGFX tft;
Audio audio; // Odtwarzacz audio
Preferences preferences; // Obiekt do przechowywania naszych ustawień w pamięci nieulotnej Flash
fft_config_t *fftCfg = nullptr;

// ------------------ E N K O D E R --------------------//
Rotary EncL = Rotary(S1_EncoderL, S2_EncoderL); // Lewy enkoder do regulacji głośności radia
Rotary EncR = Rotary(S1_EncoderR, S2_EncoderR); // Prawy enkoder do wyboru stacji
// ------------------ E N K O D E R --------------------//





// ------------------- S T A C J E ---------------------//
#define STACJE_POS_Y 30
// Lista naszych stacji. Na razie nie będziemy wnikali w konstrukcję tej struktury w języku C,
// po prostu tutaj wpisujemy listę stacji dodając nowe wiersze :)
//
// Stacje trzymane są w wierszach, a dla każdej stacji (wiersza) mamy:
// nazwa stacji to kolumna 1 (w języku C indeksy tablic rozpoczynają się od 0 a nie od 1, więc kolumna 1, tj. nazwa stacji ma indeks [0] w każdym wierszu)
// adres stacji to kolumna 2 (adres stacji ma indeks[1] w każdym wierszu)
//
//  W naszym przykładzie lista stacji ma więc format: Lista_Stacji[numer_wiersza][numer_kolumny], czyli Lista_Stacji[0...4][0...1]
//
// Przykład sla stacji RMF FM:
//  nazwa_stacji = Lista_Stacji[2][0];
//  adres_stacji = Lista_Stacji[2][1];
//
const char* Lista_Stacji[][2] = {
//                Kolumna 0      Kolumna 1                                                               
/*Wiersz 0*/    {"ITALO4YOU",   "http://s0.radiohost.pl:8018/stream"},
/*Wiersz 1*/    {"ESKA",        "https://waw.ic.smcdn.pl/2380-1.aac"},
/*Wiersz 2*/    {"RMF FM",      "https://rs101-krk-cyfronet.rmfstream.pl/RMFFM48"},
/*Wiersz 3*/    {"VOX FM",      "https://waw.ic.smcdn.pl/3990-1.aac"},
/*Wiersz 4*/    {"ESKA ROCK",   "https://waw.ic.smcdn.pl/5980-1.aac"}   // W ostatnim wierszu nie dajemy na końcu przecinka ','
};

int g_Stacja = 0; // zmienna globalna przechowująca indeks aktualnej stacji, czyli indeks wiersza w Lista_Stacji[g_Stacja][1]; w tym przypadku indeks 0, czyli stację 1 -> "Italo 4 you" :)
unsigned long g_ostatniaZmianaStacji;
bool g_aktywnaZmianaStacji = false;
bool g_streamGotoweParametry = false;
// ------------------- S T A C J E ---------------------//


// ----------------- G Ł O Ś N O Ś Ć -------------------//
int g_Volume = 3; // zmienna globalna przechowująca aktualną głośność radia
bool g_Mute = false;
unsigned long g_ostatniaZmianaVolume;
bool g_aktywnaZmianaVolume = false;
// ----------------- G Ł O Ś N O Ś Ć -------------------//

// ----------------------- V U -------------------------//
unsigned long g_czasOstatniegoNarysowaniaVU = millis(); // Zmienna, w której będziemy przechowywali czas ostatniego narysowania wskaźnika VU. Funkcja millis() zwraca liczbę milisekund, które upłynęły od momentu włączenia radia do chwili wywołania tej funkcji
// ----------------------- V U -------------------------//

// --------------------- W i F i -----------------------//
unsigned long g_czasOstatniegoStatusuWiFi = millis(); // Zmienna, w której będziemy przechowywali czas ostatniego wyświetlenia statusu WiFi
// --------------------- W i F i -----------------------//


// --------- A N A L I Z A T O R   W I D M A ------------//
volatile bool g_audio_process_i2s_Copy_In_Progress = false;
volatile bool g_calculateFFT_Copy_In_Progress = false;

int16_t bufferI2S[2 * SIZE_OF_FFT] = {0};
unsigned long g_czasStartWyswietlAnalizatorWidma = millis(); // Zmienna globalna przechowująca moment (w ms od uruchomienia radia), w którym wyświetlono ostatni obraz wygaszacza ekranu
float spectrum[SIZE_OF_FFT / 2];

void drawSpectrum16(float *spectrum, int xl, int yl, int width, int height, float colGapPercent, float segGapPercent, bool reset); // Deklaracja funkcji drawSpectrum16
void drawSpectrum16Invert(float *spectrum, int xl, int yl, int width, int height, float colGapPercent, float segGapPercent, bool reset); // Deklaracja funkcji drawSpectrum16Invert
// --------- A N A L I Z A T O R   W I D M A ------------//



// Inne zmienne globalne
String g_streamTitle = ""; // Zmienna globalna przechowująca aktualny tytuł streamu
String g_codecName = ""; // Zmienna globalna przechowująca nazwę aktualnego kodeka streamu
int g_bitRate = 0; // Zmienna globalna przechowująca bitrate aktualnego streamu
int g_sampleRate = 0; // Zmienna globalna przechowująca samp[le rate aktualnego streamu
unsigned long g_czasStartWygaszaczTimeout = millis(); // Zmienna globalna przechowująca moment (w ms od uruchomienia radia), w którym rozpoczęło się odliczanie czasu do aktywacji wygaszacza
unsigned long g_czasStartWygaszaczImage = millis(); // Zmienna globalna przechowująca moment (w ms od uruchomienia radia), w którym wyświetlono ostatni obraz wygaszacza ekranu
int g_trybPracyRadia = MODE_MAIN;
int g_imageNumber = 1;






// Funkcja pomocnicza służąca do obliczania wysokości całkowitej tekstu
// Nie będziemy analizować jej działania - musicie uwierzyć na słowo, że działa :)
int obliczWysokoscTekstu(String tekst) {
  if (tekst.length() == 0) return 0;
  int maxSzerokosc = 320;
  int h = tft.fontHeight();
  int liczbaLinii = 1;
  int xCursor = 0;
  String slowo = "";
  int i = 0;
  while (i < tekst.length()) {
    if (tekst[i] == ' ') {
      int spacjaW = tft.textWidth(" ");
      if (xCursor + spacjaW > maxSzerokosc) {
        liczbaLinii++;
        xCursor = 0;
      } else {
        xCursor += spacjaW;
      }
      i++;
      continue;
    }
    slowo = "";
    while (i < tekst.length() && tekst[i] != ' ') {
      slowo += tekst[i];
      i++;
    }
    int slowoW = tft.textWidth(slowo);
    if (xCursor + slowoW > maxSzerokosc) {
      liczbaLinii++;
      xCursor = slowoW; 
    } else {
      xCursor += slowoW;
    }
  }
  return liczbaLinii * h;
}




// Opcjonalna funkcja wyświetlająca komunikaty odtwarzacza audio w Serial Monitorze oraz nazwę streamu na wyświetlaczu
void my_audio_info(Audio::msg_t m) {
  Serial.printf("%s: %s\n", m.s, m.msg);
  if (m.e == Audio::evt_streamtitle) { // Biblioteka Audio I2S zwróciła tytuł streamu
    g_streamTitle = String(m.msg);
    g_streamTitle.trim(); // Ucinamy ew. spacje na początku i końcu tutułu streamu
    if (g_streamTitle.length() > 75) g_streamTitle = g_streamTitle.substring(0, 75); // ograniczamy tytuł streamu do maksymalnie 3 linii (ok. 75 znaków).
    if (g_trybPracyRadia == MODE_MAIN) {
      tft.fillRect(0, 105, 320, 75, TFT_BLACK); // Czyścimy obszar tytułu streamu
      tft.setTextColor(TFT_CYAN); // Ustawiamy kolor tekstu na cyjanowy
      int tytulStreamu_Y; // Zmienna, w której będziemy przechowywać współrzędną pionową Y tytułu streamu
      int wysokoscTytuluStreamu = obliczWysokoscTekstu(g_streamTitle); // Obliczamy całkowitą wysokość tytułu streamu, także wieloliniowego 
      if (wysokoscTytuluStreamu > 50) tytulStreamu_Y = 130; // tytuł streamu składa się z 3 linii 
      else if (wysokoscTytuluStreamu > 25) tytulStreamu_Y = 142; // tytuł streamu składa się z 2 linii 
      else tytulStreamu_Y = 154; // tytuł streamu składa się z 1 linii 
      tft.setCursor(0, tytulStreamu_Y); // Ustawiamy kursor na pozycji (0, tytulStreamu_Y)
      tft.setTextDatum(BL_DATUM); // Ustawiamy punkt kotwiczenia tekstu (dolny lewy wierzchołek tekstu)
      tft.print(g_streamTitle); // Wyświetlamy nowy tytuł streamu
    }
  }
  else if (m.e == Audio::evt_info) { // W informacji zwróconej przez bibliotekę Audio I2S, szukamy  tekstu "BitsPerSample:",
                                     // co znaczy, ze gotowe do pobrania są już parametry streamu
    String info(m.msg);
    if (String(info).indexOf("BitsPerSample:") != -1) g_streamGotoweParametry = true; // Pojawiły się nowe parametry streamu, więc możemy je pobrać i wyświetlić (g_streamGotoweParametry = true) 
  }
}


// --------------------- L O G O -----------------------//
// Ta funkcja będzie wyświetlała logo stacji o nazwie nazwaStacji przekazanej jako parametr w wywołaniu funkcji
void wyswietlLogoStacji(String nazwaStacji) { // do funkcji przekazujemy nazwę stacji, dla której chcemy wyświetlić logo
  // Przyjmujemy następujące współrzędne środka logo:
  // x_sr = 230
  // y_sr = 65
  tft.fillRect(170, 35, 120, 60, TFT_BLACK); // Czyścimy poprzednie logo przed wyświetleniem nowego
  if (nazwaStacji == "ITALO4YOU") tft.pushImage(200, 35, 60, 60, ITALO4YOU);
  else if (nazwaStacji == "ESKA") tft.pushImage(170, 45, 120, 40, ESKA);
  else if (nazwaStacji == "RMF FM") tft.pushImage(183, 35, 95, 60, RMFFM);
  else if (nazwaStacji == "VOX FM") tft.pushImage(200, 35, 60, 60, VOXFM);
  else if (nazwaStacji == "ESKA ROCK") tft.pushImage(195, 35, 70, 60, ESKAROCK);
  else tft.pushImage(197, 35, 67, 60, RADIO);
}
// --------------------- L O G O -----------------------//



// ----------------------- V U -------------------------//
// Funkcja rysująca wskaźnik VU na pozycji (x, y) <- współrzędne lewego dolnego rogu wskaźnika
void drawVU(uint16_t x, uint16_t y) {
  // Parametry wskaźnika VU
  const uint8_t  numSegments = 16; // Liczb segmentów wskaźnika VY
  const uint8_t  segW = 8; // Szerokość segmentu
  const uint8_t  segH = 8;  // Wysokość segmentu
  const uint8_t  gap  = 2;  // Odstęp między segmentami

  // Pobranie poziomów dźwięku
  uint16_t levels = audio.getVUlevel();
  uint8_t left    = levels >> 8;   // poziom lewego kanału trzymany jest w górnym bajcie zmiennej levels (wartości 0...255)
  uint8_t right   = levels & 0xFF; // poziom lewego kanału trzymany jest w dolnym bajcie zmiennej levels (wartości 0...255)

  // Tablica progów - po przekroczeniu danego progu przez poziom kanału, zapalany jest odpowiadający mu segment, np. dla left=15 zapalone zostaną segmenty: 1, 2, 3 i 4 (5 już się nie zapali, bo dla niego próg wynosi 16)
  static const uint8_t thresholds[16] = {2, 4, 7, 11, 16, 23, 32, 45, 62, 84, 112, 145, 182, 215, 238, 255};

  tft.setFont(&fonts::Font0); // Ustawiamy podstawową czcionkę, którą wyświetlimy litery "L" i "P"
  tft.setTextSize(1); // Ustawiamy rozmiar czcionki na 1
  tft.setTextColor(TFT_WHITE); // Ustawiamy kolor czcionki na biały
  tft.setTextDatum(BL_DATUM); // Ustawiamy kotwiczenie tekstu (dolny lewy wierzchołek tekstu)
  tft.drawString("L", VU_POS_X - 10, VU_POS_Y - 2 - 8 + 1); // Przed paskiem lewego kanału wyświetlamy literę "L"
  tft.drawString("P", VU_POS_X - 10, VU_POS_Y + 1); // Przed paskiem prawego kanału wyświetlamy literę "P"
  tft.loadFont(Lato_Bold_22); // Przywracamy czcionkę Lato_Bold_22 (tekst od tej pory, do kolejnej zmiany będzie wyświetlany z użyciem tej czcionki)

  for (int i = 0; i < numSegments; i++) { // Główna pętla wyświetlająca segmenty dla obu kanałów; przechodzimy przez wszystkie 16 segmentów
                                          // (ale pamiętamy, że w języku C++ indeksy tablic zaczynają się od zera, wiec pierwszy element ma indeks 0, a ostatni - 15)
    uint16_t curX = x + (i * (segW + gap)); // Obliczamy pozycję X dla bieżącego segmentu
    uint16_t color; // Kolor segmentu jest zależny od jego pozycji (zielony -> żółty -> czerwony)
    if (i < 10)      color = 0x07E0; // 10 segmentów zielonych (indeksy 0-9)
    else if (i < 14) color = 0xFFE0; // 4 segmenty zółte (indeksy 10-13)
    else            color = 0xF800; // 2 segmenty czerwone (indeksy 14-15)

    // Kanał lewy (górny pasek)
    // Rysujemy segment, jeśli poziom jest powyżej progu, w przeciwnym razie tło
    bool leftOn = (left >= thresholds[i]); 
    tft.fillRect(curX, y - (segH * 2 + gap), segW, segH, leftOn ? color : 0x0000);
    // Kanał prawy (dolny pasek)
    // Rysujemy segment, jeśli poziom jest powyżej progu, w przeciwnym razie tło
    bool rightOn = (right >= thresholds[i]); 
    tft.fillRect(curX, y - segH, segW, segH, rightOn ? color : 0x0000);
  }
}
// ----------------------- V U -------------------------//



// --------------------- W i F i -----------------------//
// Funkcja wyświetla poziom sygnału WiFi
void wyswietlStatusWiFi() {
  int8_t poziomWiFi = WiFi.RSSI(); // Pobieramy aktualny poziom sygnału WiFi
  tft.setTextColor(KOLOR_WIFI, TFT_BLACK); // Ustawiamy kolor tekstu na zdefiniowany KOLOR_WIFI na czarnym tle
  char cVol[7]; // zmienna pomocnicza
  snprintf(cVol, sizeof(cVol), "%3ddBm", poziomWiFi); // Format wyświetlania "%3d" powoduje, że jeżeli liczba jest krótsza niż 3 znaki,
                                                      // to zostaną doda spacje z lewej strony tak, aby dopełnić rozmiar tekstu do 3 znaków.
  tft.setTextDatum(BL_DATUM); // ustawiamy punkt kotwiczenia tekstu (dolny lewy wierzchołek tekstu)
  tft.drawString(cVol, 0, WIFI_POS_Y); // Wyświetlamy aktualną wartość głośności na pozycji (0, WIFI_POS_Y);
  uint16_t kolorWskaznikaSygnalu;
  if (poziomWiFi <= -85) kolorWskaznikaSygnalu = TFT_RED; // jeżeli poziom jest niższy niż -85dBm, to wskaźnik rysujemy na czerwono
  else if (poziomWiFi <= -75) kolorWskaznikaSygnalu = TFT_ORANGE; // jeżeli poziom jest wyższy niż -85dBm, ale niższy niż -75 dbM, to wskaźnik rysujemy na pomarańczowo
  else kolorWskaznikaSygnalu = KOLOR_WIFI; // // jeżeli poziom jest wyższy niż -75dBm, to wskaźnik rysujemy zdefiniowanym kolorem KOLOR_WIFI
  const uint16_t x_start = 85; // współrzędna X wskaźnika spoziomu sygnału
  const uint16_t y_base = WIFI_POS_Y - 4; // // współrzędna Y wskaźnika spoziomu sygnału
  const uint8_t bar_width = 5; // szerokość pasków we wskaźniku sygnału
  const uint8_t bar_spacing = 8; // odstęp pomiędzy paskami wskaźnika sygnału
  const int progi[3] = {-85, -75, -65}; // progi rysowania wypełnionych pasków we wskaźniku sygnały WiFi
  const int wysokosci[3] = {4, 9, 14}; // wysokości poszczególnych pasków we wskaźniku sygnały WiFi
  for (int i = 0; i < 3; ++i) { // Rysujemy 3 paski wskaźnika sygnału
    uint16_t current_x = x_start + (i * bar_spacing); // Obliczamy aktualną współrzędną danego paska
    uint16_t current_y = y_base - wysokosci[i]; // Obliczamy wysokość danego paska
    if (poziomWiFi >= progi[i]) tft.fillRect(current_x, current_y, bar_width, wysokosci[i], kolorWskaznikaSygnalu); // Jeżeli poziom sygnału WiFi jest wyższy niż próg zdefiniowany dla danego paska, rysujemy pełny słupek,
    else { // w przeciwnym razie rysujemy obramowanie słupka bez wypełnienia
      tft.drawRect(current_x, current_y, bar_width, wysokosci[i], kolorWskaznikaSygnalu); // Obramowanie
      tft.fillRect(current_x + 1, current_y + 1, bar_width - 2, wysokosci[i] - 2, TFT_BLACK); // Środek wypełniamy kolorem czarnym
    }
  }
}



// Funkcja wyświetla nazwę sieci WiFi
void wyswietlNazweWiFi() {
  String nazwaSieci = String(WiFi.SSID()); // Pobieramy nazwę sieci
  if (nazwaSieci.length() > 15) nazwaSieci = nazwaSieci.substring(0, 15); // Ograniczamy długość wyświetlanej nazwy sieci do 15 znaków
  tft.setTextColor(KOLOR_WIFI); // Ustawiamy kolor czcionki na KOLOR_WIFI
  tft.setTextDatum(BL_DATUM); // Ustawiamy punkt kotwiczenia tekstu (dolny lewy wierzchołek tekstu)
  tft.drawString(nazwaSieci.c_str(), SSID_POS_X, WIFI_POS_Y);  // Wyświetlamy nazwę sieci WiFi na pozycji (SSID_POS_X, WIFI_POS_Y)
}
// --------------------- W i F i -----------------------//





// Funkcja przywraca elementy głównego ekranu przy wyjściu z trybu wygaszacza ekranu
void powrotDoGlownegoEkranu() {
  tft.fillScreen(TFT_BLACK); // Czyścimy ekran - kolor czarny
  tft.setFont(&fonts::FreeSansBold12pt7b); // Ustawienie czcionki FreeSansBold12pt7b
  tft.setTextColor(TFT_ORANGE); // Ustawiamy kolor czcionki wyświetlacza
  tft.setTextDatum(TC_DATUM); // Ustawiamy punkt kotwiczenia tekstu (górny środek tekstu)
  tft.drawString("RADIO INTERNETOWE", 160, 0);  //wyświetlamy wycentrowany napis "RADIO INTERNETOWE"
  tft.loadFont(Lato_Bold_22); // Ustawiamy czcionkę na Lato_Bold_22
  tft.setTextDatum(TL_DATUM); // Ustawiamy punkt kotwiczenia tekstu (górny lewy wierzchołek tekstu)
  tft.setTextColor(TFT_MAGENTA); tft.drawString(Lista_Stacji[g_Stacja][0], 0, STACJE_POS_Y); // Zmieniamy kolor czcionki na magenta i wyświetlamy zdefiniowaną wcześniej nazwę stacji
                                                                                             // na pozycji (0, STACJE_POS_Y).
  
  tft.setFont(&fonts::FreeSans9pt7b); // Ustawiamy czcionkę na FreeSans9pt7b
  tft.setTextColor(0xf68d, TFT_BLACK); // ustawiamy kolor tekstu na 0xf68d na czarnym tle
  tft.setTextDatum(BL_DATUM); // Ustawiamy punkt kotwiczenia tekstu (dolny lewy wierzchołek tekstu)
  char cCodecBitRate[15]; // zmienna pomocnicza
  snprintf(cCodecBitRate, sizeof(cCodecBitRate), "%s %dk", g_codecName, g_bitRate); // Format wyświetlania "%02d" powoduje, że zawsze wyświetlana jest liczba 2-cyfrowa, np. 01
  tft.drawString(cCodecBitRate, 0, 80); // wyświetlamy nazwę kodeka (tj. MP3, AAC, FLAC, itd.) na pozycji (0, 80)
  char cSampleRate[10]; // zmienna pomocnicza
  snprintf(cSampleRate, sizeof(cSampleRate), "%dHz", g_sampleRate); // Format wyświetlania "%02d" powoduje, że zawsze wyświetlana jest liczba 2-cyfrowa, np. 01
  tft.drawString(cSampleRate, 0, 100); // wyświetlamy Sample Rate (np. 44100, 48000, etc.) na pozycji (0, 100)
  
  tft.loadFont(Lato_Bold_22); // Przywracamy czcionkę Lato_Bold_22
  tft.setTextColor(TFT_CYAN); // Ustawiamy kolor tekstu na cyjanowy
  int tytulStreamu_Y; // Zmienna, w której będziemy przechowywać współrzędną pionową Y tytułu streamu
  int wysokoscTytuluStreamu = obliczWysokoscTekstu(g_streamTitle); // Obliczamy całkowitą wysokość tytułu streamu, także wieloliniowego 
  if (wysokoscTytuluStreamu > 50) tytulStreamu_Y = 130; // tytuł streamu składa się z 3 linii 
  else if (wysokoscTytuluStreamu > 25) tytulStreamu_Y = 142; // tytuł streamu składa się z 2 linii 
  else tytulStreamu_Y = 154; // tytuł streamu składa się z 1 linii 
  tft.setCursor(0, tytulStreamu_Y); // Ustawiamy kursor na pozycji (0, tytulStreamu_Y)
  tft.setTextDatum(BL_DATUM); // Ustawiamy punkt kotwiczenia tekstu (górny lewy wierzchołek tekstu)
  tft.print(g_streamTitle); // Wyświetlamy nowy tytuł streamu

  wyswietlLogoStacji(Lista_Stacji[g_Stacja][0]); // Wyświetlamy logo pierwszej stacji
  wyswietlStatusWiFi(); // Wyświetlamy status WiFi
  wyswietlNazweWiFi(); // Wyswietlamy nazwę stacji WiFi
  
  if (!g_Mute) {
    tft.setTextColor(VOLUME_KOLOR); //Ustawiamy kolor tekstu na VOLUME_KOLOR
    tft.setTextDatum(BL_DATUM); // Ustawiamy punkt kotwiczenia tekstu (dolny lewy wierzchołek tekstu)
    char cVol[7]; // zmienna pomocnicza
    snprintf(cVol, sizeof(cVol), "Vol:%02d", g_Volume); // Format wyświetlania "%02d" powoduje, że zawsze wyświetlana jest liczba 2-cyfrowa, np. 01
    tft.drawString(cVol, 0, 240); // Wyświetlamy aktualną wartość głośności na pozycji (48, 225);
  }
  else {
    tft.setTextColor(TFT_WHITE, TFT_RED); // Zmieniamy kolor czcionki na biały na czerwonym tle
    tft.setTextDatum(BL_DATUM); // Ustawiamy punkt kotwiczenia tekstu (dolny lewy wierzchołek tekstu)
    char cVol[7]; // zmienna pomocnicza
    snprintf(cVol, sizeof(cVol), "Vol:%02d", g_Volume); // Format wyświetlania "%02d" powoduje, że zawsze wyświetlana jest liczba 2-cyfrowa, np. 01
    tft.drawString(cVol, 0, 240); // Wyświetlamy aktualną wartość głośności na pozycji (48, 225);
  }
}





// Funkcja wyświetla naprzemiennie obrazy w trybie wygaszacza ekranu
void wyswietlObraz() {
  const int imageWidth = 160; // Szerokość obrazów w trybie wygaszacza ekranu
  const int imageHeight = 120; // Wysokość obrazów w trybie wygaszacza ekranu
  int minX = 0; // Wartość minimalna współrzędnej X górnego lewego wierzchołka obrazu
  int maxX = 320 - imageWidth; // Wartość maksymalna współrzędnej X górnego lewego wierzchołka obrazu (dobierana tak, żeby zmieścił się cały obraz)
  int minY = 0; // // Wartość minimalna współrzędnej Y górnego lewego wierzchołka obrazu
  int maxY = 240 - imageHeight; // Wartość maksymalna współrzędnej Y górnego lewego wierzchołka obrazu (dobierana tak, żeby zmieścił się cały obraz)
  int x = random(minX, maxX + 1); // Losujemy współrzędną X górnego lewego wierzchołka obrazu
  int y = random(minY, maxY + 1); // Losujemy współrzędną Y górnego lewego wierzchołka obrazu   

  tft.fillScreen(TFT_BLACK); // Czyścimy ekran - kolor czarny
  if (g_imageNumber < 4) g_imageNumber++; // Przy każdym wywołaniu funkcji zwiększamy numer obrazu do wyświetlenia
  else g_imageNumber = 1; // Jeżeli wyświetliliśmy ostatni obraz, to zaczynamy od pierwszego 
  switch (g_imageNumber) {
    case 1:
      tft.pushImage(x, y, imageWidth, imageHeight, IMAGE1);
      break;
    case 2:
      tft.pushImage(x, y, imageWidth, imageHeight, IMAGE2);
      break;  
    case 3:
      tft.pushImage(x, y, imageWidth, imageHeight, IMAGE3);
      break;
    case 4:
      tft.pushImage(x, y, imageWidth, imageHeight, IMAGE4);
      break;
  }
}



/*************************** U W A G A ***************************/
//
// W zależności od posiadanej wersji biblioteki ESP32-audioI2S (16 lub 32-bitowej)
// trzeba odkomentować właściwą funkcję
//

//
// U W A G A: Poniższa wersja funkcji audio_process_i2s wymaga
//            16-bitowej biblioteki ESP32-audioI2S 
//
// Funkcja zwraca próbki I2S dźwięku
void audio_process_i2s(int16_t* outBuff, int32_t validSamples, bool* continueI2S) {
  if (g_trybPracyRadia == MODE_SPECTRUM_ANALYZER_1 || g_trybPracyRadia == MODE_SPECTRUM_ANALYZER_2) { // Jeżeli jestesmy w trybie analizatora widma
    if (validSamples >= 2 * SIZE_OF_FFT) {          // i dostępnych jest więcej lub równo 2*SIZE_OF_FFT próbek, to
      while (g_calculateFFT_Copy_In_Progress == true) { vTaskDelay(1); } // Jeżeli trwa kopiowanie danych w funkcji calculateFFT,
                                                                         // to czekamy na jego zakończenie.
      g_audio_process_i2s_Copy_In_Progress = true; // Ustawienie blokady kopiowania danych dla funkcji calculateFFT
      memcpy(bufferI2S, outBuff, 2 * SIZE_OF_FFT * sizeof(int16_t)); //kopiujemy próbki do naszej tablicy "bufferI2S",
                                                                     // która posłuży do obliczania transformaty FFT.
                                                                     // Tablica zawiera naprzemiennie próbki kanału lewego i prawego.
      g_audio_process_i2s_Copy_In_Progress = false; // Zdjęcie blokady kopiowania danych dla funkcji calculateFFT
    }  
  }
  *continueI2S = true; // Dajemy znać bibliotece Audio I2S, że może przesyłać próbki dźwięku dalej, czyli do DACa
}




// U W A G A: Poniższa wersja funkcji audio_process_i2s wymaga
//            32-bitowej biblioteki ESP32-audioI2S 
/*
// Funkcja zwraca próbki I2S dźwięku
void audio_process_i2s(int32_t* outBuff, int16_t validSamples, bool* continueI2S) {
  if (g_trybPracyRadia == MODE_SPECTRUM_ANALYZER_1 || g_trybPracyRadia == MODE_SPECTRUM_ANALYZER_2) { // Jeżeli jestesmy w trybie analizatora widma
    if (validSamples >= 2 * SIZE_OF_FFT) {          // i dostępnych jest więcej lub równo 2*SIZE_OF_FFT próbek, to
      while (g_calculateFFT_Copy_In_Progress == true) { vTaskDelay(1); } // Jeżeli trwa kopiowanie danych w funkcji calculateFFT,
                                                                         // to czekamy na jego zakończenie.
      g_audio_process_i2s_Copy_In_Progress = true; // Ustawienie blokady kopiowania danych dla funkcji calculateFFT
      for (size_t i = 0; i < 2 * SIZE_OF_FFT; i++) {
        bufferI2S[i] = (int16_t)((*outBuff) >> 16);
        outBuff++; 
      }
	    g_audio_process_i2s_Copy_In_Progress = false; // Zdjęcie blokady kopiowania danych dla funkcji calculateFFT
    }  
  }
  *continueI2S = true; // Dajemy znać bibliotece Audio I2S, że może przesyłać próbki dźwięku dalej, czyli do DACa
}
*/

/*************************** U W A G A ***************************/





// Funkcja oblicza szybką transformatę Fouriera
// i zapisuje wynik w tablicy spectrum, która posłuży do rysowania Analizatora Widma
void calculateFFT() {
  int fftIndex = 0;
	float fftInput[SIZE_OF_FFT]; // Tablica wejściowa do obliczenia transformaty FFT 
  
  while (g_audio_process_i2s_Copy_In_Progress == true) { vTaskDelay(1); } // Jeżeli trwa kopiowanie danych dźwięku I2S w funkcji audio_process_i2s,
                                                                          // to czekamy na jego zakończenie
  g_calculateFFT_Copy_In_Progress = true; // Ustawienie blokady kopiowania danych dla funkcji audio_process_i2s
  for (int i = 0; i < 2 * SIZE_OF_FFT; i += 2) { // Iterujemy po wszystkich próbkach dźwięku, tj. kanał L i P
    int16_t left = bufferI2S[i]; // Bierzemy próbkę kanału lewego
    int16_t right = bufferI2S[i + 1]; // Bierzemy próbkę kanału prawego 
    float mono = (left + right) / 2.0f; // Liczymy uśrednioną próbkę mono
    fftInput[fftIndex++] = mono; // Zapisujemy próbkę mono do tablicy wejściowej do obliczenia transformaty FFT
  }
  g_calculateFFT_Copy_In_Progress = false; // Zdjęcie blokady kopiowania danych dla funkcji audio_process_i2s
  
  // Skopiuj dane do bufora wejściowego FFT
  for (int i = 0; i < SIZE_OF_FFT; i++) { // Iterujemy po wszystkich próbkach mono
    float window = 0.54f - 0.46f * cosf(2.0f * M_PI * i / (SIZE_OF_FFT - 1));  // Przed obliczeniem FFT, stosujemy okno Hamminga
    fftCfg->input[i] = fftInput[i] * window; // j.w.
	}
  fft_execute(fftCfg); // Obliczamy FFT
  // Obliczamy amplitudy widma i zapisujemy w tablicy spectrum
  for (int i = 0; i < SIZE_OF_FFT / 2; i++) { // Bierzemy tylko połowę wyniku fransformaty FFT, ponieważ dla danych audio
                                              // druga połowa transformaty FFT jest symetrycznym odbiciem pierwszej połowy
    float real = fftCfg->output[i * 2]; // Obliczamy składową rzeczywistą wyniku transformaty FFT
    float imag = fftCfg->output[i * 2 + 1]; // Obliczamy składową urojoną wyniku transformaty FFT
    spectrum[i] = sqrtf(real * real + imag * imag); // Obliczamy amplitudę widma
  }
}




// Funkcja oblicza transformatę FFT i wyświetla Analizator Widma
void wyswietlAnalizatorWidma() { 
  calculateFFT();
  if (g_trybPracyRadia == MODE_SPECTRUM_ANALYZER_1) drawSpectrum16(spectrum, 0, 240, 320, 180, 0.2f, 0.4f, false); //0.1f
  else if (g_trybPracyRadia == MODE_SPECTRUM_ANALYZER_2) drawSpectrum16Invert(spectrum, 0, 0, 320, 180, 0.2f, 0.4f, false);
}




// Funkcja setup
void setup() {
  pinMode(TFT_BL, OUTPUT); // Ustawienie pinu odpowiedzialnego za podświetlenie wyświetlacza jako wyjście
  digitalWrite(TFT_BL, LOW); // Wyłączenie podświetlenia wyświetlacza, żeby uniknać artefaktów podczas inicjalizacji

  // Ustawienie pinów GPIO, do których podłączone są enkodery, jako wejścia
  pinMode(S1_EncoderR, INPUT_PULLUP);
  pinMode(S2_EncoderR, INPUT_PULLUP);
  pinMode(S1_EncoderL, INPUT_PULLUP);
  pinMode(S2_EncoderL, INPUT_PULLUP);
  pinMode(buttonEncoderL, INPUT_PULLUP);
  pinMode(buttonEncoderR, INPUT_PULLUP);
  
  Serial.begin(115200); // Inicjalizacja monitora portu szeregowego, tj. Serial Monitor

  // -------- P R E F E R E N C E S --------//
  preferences.begin("ustawienia", false); // Inicjalizacja obiektu oreferences do przechowywania ustawień radia w pamięciu nieulotnej Flash
  g_Volume = preferences.getUChar("volume", 5); // odczyt zapisanej w pamięci Flash ostatnio ustawionej głośności (odczyt liczby dodatniej 8-bitowej); jeżeli nic nie zapisano zwracana jest wartość 5.
  g_Stacja = preferences.getUChar("stacja", 0); // odczyt zapisanej w pamięci Flash ostatnio wybranej stacji (odczyt liczby dodatniej 8-bitowej); jeżeli nic nie zapisano zwracana jest wartość 0 (czyli pierwsza stacja).
  // -------- P R E F E R E N C E S --------//

  WiFi.begin("nazwa_sieci_wifi", "hasło_sieci_wifi"); // Podłączenie do sieci WiFi -> WAŻNE: trzeba wpisać swoje parametry sieci WiFi
  
  spi.begin(TFT_SCLK, -1, TFT_MOSI, TFT_CS); // Inicjalizacja interfejsu SPI
    
  
  tft.init(); // Inicjalizacja wyświetlacza
  tft.setSwapBytes(true); // Dostosowanie poprawnego wyświetlania bitmap;
                          // jeżeli ibrazy wyswietlane są niepoprawnie trzeba zmienićna false
  //
  // !!! Poniżej trzeba pozostawić aktywną tylko jedną sekcję, w zależności od posiadanego wyświetlacza !!!
  // ---------- ST7789 ----------//
  tft.setRotation(3); // Dopasowanie obrotu obrazu
  //
  // ---------- ILI9341 ---------//
  //tft.setRotation(1); // Dopasowanie obrotu obrazu


  
  tft.setTextWrap(true); // Tekst dłuższy niż szerokość ekranu będzie zawijany do kolejnej linii
  tft.fillScreen(TFT_BLACK); // Czyścimy ekran - kolor czarny

  // --------------------- L O G O -----------------------//
  // Do wyświetlania bitmap będziemy używać funkcji z biblioteki LovyanGFX:
  // pushImage(x_lewy_górny_wierzchołek_logo, y__lewy_górny_wierzchołek_logo, szerokosc_bitmapy, wysokosc_bitmapy, *bitmapa);
  tft.setFont(&fonts::FreeSansBold12pt7b); // Ustawienie czcionki FreeSansBold12pt7b
  tft.setTextColor(TFT_ORANGE); // Ustawiamy kolor czcionki wyświetlacza
  tft.setTextDatum(TC_DATUM); // Ustawiamy punkt kotwiczenia tekstu (górny środek tekstu)
  tft.drawString("NAJPROSTSZE", 160, 0); // Wyświetlamy napis "NAJPROSTSZE" na pozycji (160, 0) - ponieważ punkt kotwiczenia ustalony jest jako górny środek tekstu, to napis będzie wycentrowany na ekranie
  tft.drawString("RADIO INTERNETOWE", 160, 30); // j.w.
  tft.pushImage(20, 77, 280, 141, LOGO); // Wyświetlamy logo startowe; dobieramy współrzedne lewego górnego wierzchołka tak, żeby logo było wycentrowane
  digitalWrite(TFT_BL, HIGH); // Włączamy podświetlenie wyświetlacza
  delay(5000); // Przez 5 sekund podziwiamy logo :)
  tft.fillScreen(TFT_BLACK); // Czyścimy ekran - kolor czarny
  // --------------------- L O G O -----------------------//

  tft.drawString("RADIO INTERNETOWE", 160, 0);  // j.w. - wyświetlamy wycentrowany napis "RADIO INTERNETOWE"
  tft.loadFont(Lato_Bold_22); // Ustawiamy czcionkę na Lato_Bold_22
  tft.setTextDatum(TL_DATUM); // Ustawiamy punkt kotwiczenia tekstu (górny lewy wierzchołek tekstu)
  tft.setTextColor(TFT_MAGENTA); tft.drawString(Lista_Stacji[g_Stacja][0], 0, STACJE_POS_Y); // Zmieniamy kolor czcionki na magenta i wyświetlamy zdefiniowaną wcześniej nazwę stacji
                                                                                             // na pozycji (0, STACJE_POS_Y).
  
  // --------------------- L O G O -----------------------//
  wyswietlLogoStacji(Lista_Stacji[g_Stacja][0]); // Wyświetlamy logo pierwszej stacji
  // --------------------- L O G O -----------------------//
  
  // Czekamy na połączenie z siecią WiFi
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print("*");
    delay(100);
  }
  Serial.println("\nWiFi Połączone :)");

  // --------------------- W i F i -----------------------//
  wyswietlStatusWiFi(); // Wyświetlamy status WiFi
  wyswietlNazweWiFi(); // Wyswietlamy nazwę stacji WiFi
  // --------------------- W i F i -----------------------//

  // Ustawiamy konfigurację odtwarzacza Audio
  audio.setPinout(I2S_BCLK, I2S_LRC, I2S_DOUT); // Informujemy odtwarzacz o tym, jakich pinów ma używać
  Audio::audio_info_callback = my_audio_info; // Opcjonalnie - funkcja wyświetla szczegóły połączenie na Serial Monitorze i tytułstreamu na wyświetlaczu
  audio.setAudioTaskCore(1);

  // --------- A N A L I Z A T O R   W I D M A ------------//
  fftCfg = fft_init(SIZE_OF_FFT, FFT_REAL, FFT_FORWARD, nullptr, nullptr);
  // --------- A N A L I Z A T O R   W I D M A ------------//

  // ----------------- G Ł O Ś N O Ś Ć -------------------//
  audio.setVolume(g_Volume); // Ustawiamy głośność dźwięku (standardowy zakres 0...21)
  tft.setTextColor(VOLUME_KOLOR); //Ustawiamy kolor tekstu na VOLUME_KOLOR
  tft.setTextDatum(BL_DATUM); // Ustawiamy punkt kotwiczenia tekstu (dolny lewy wierzchołek tekstu)
  char cVol[7]; // zmienna pomocnicza
  snprintf(cVol, sizeof(cVol), "Vol:%02d", g_Volume); // Format wyświetlania "%02d" powoduje, że zawsze wyświetlana jest liczba 2-cyfrowa, np. 01
  tft.drawString(cVol, 0, 240); // Wyświetlamy aktualną wartość głośności na pozycji (48, 225);
  // ----------------- G Ł O Ś N O Ś Ć -------------------//
  
  audio.connecttohost(Lista_Stacji[g_Stacja][1]); // podłączamy się do stacji, której indeks jest zdefiniowany w zmiennej g_Stacja, czyli teraz 0 (tj. pierwsza stacja),
                                                  // a [1] oznacza, że bierzemy 2 kolumnę z wiersza [g_stacja] (pamiętamy, że indeksy są o 1 mniejsze niż rzeczywiste numery kolumn i wierszy),
                                                  // po czym powinniśmy usłyszeć dźwięk (szczegóły połączenie możemy obserwować na Serial Monitorze)
  
  g_czasStartWygaszaczTimeout = millis(); // Aktualizujemy zmienną przechowującą moment, w którym rozpoczęło się odliczanie czasu do aktywacji wygaszacza
}


// Główna pętla loop
void loop() {
  // Wyświetlanie parametrów streamu
  if (g_trybPracyRadia == MODE_MAIN && g_streamGotoweParametry) { // Jeżeli parametry streamu są już dostępne, to
    tft.setFont(&fonts::FreeSans9pt7b); // Ustawiamy czcionkę na FreeSans9pt7b
    tft.setTextColor(0xf68d, TFT_BLACK); // ustawiamy kolor tekstu na 0xf68d na czarnym tle
    tft.setTextDatum(BL_DATUM); // Ustawiamy punkt kotwiczenia tekstu (dolny lewy wierzchołek tekstu)
    char cCodecBitRate[15]; // zmienna pomocnicza
    g_codecName = audio.getCodecname();
    g_bitRate = (int)audio.getBitRate()/1000L;
    snprintf(cCodecBitRate, sizeof(cCodecBitRate), "%s %dk", g_codecName, g_bitRate); // Format wyświetlania "%02d" powoduje, że zawsze wyświetlana jest liczba 2-cyfrowa, np. 01
    tft.drawString(cCodecBitRate, 0, 80); // wyświetlamy nazwę kodeka (tj. MP3, AAC, FLAC, itd.) na pozycji (0, 80)
    char cSampleRate[10]; // zmienna pomocnicza
    g_sampleRate = (int)audio.getSampleRate();
    snprintf(cSampleRate, sizeof(cSampleRate), "%dHz", g_sampleRate); // Format wyświetlania "%02d" powoduje, że zawsze wyświetlana jest liczba 2-cyfrowa, np. 01
    tft.drawString(cSampleRate, 0, 100); // wyświetlamy Sample Rate (np. 44100, 48000, etc.) na pozycji (0, 100)
    tft.loadFont(Lato_Bold_22); // Przywracamy czcionkę Lato_Bold_22
  }
  
  
  //------------------ B U T T O N S --------------------//
  // Lewy enkoder
  if (digitalRead(buttonEncoderL) == LOW) { // Jeżeli przycisk lewego enkodera jest wciśnięty, to...
    if (g_trybPracyRadia != MODE_MAIN) { // Jeżeli nie jesteśmy w trybie głównego ekranu, to  
      g_trybPracyRadia = MODE_MAIN; // przechodzimy do trybu głównego ekranu
      powrotDoGlownegoEkranu(); // i wyświetlamy niezbędne elementy głównego ekranu
    }
    g_czasStartWygaszaczTimeout = millis(); // Aktualizujemy zmienną przechowującą moment, w którym rozpoczęło się odliczanie czasu do aktywacji wygaszacza
    while (digitalRead(buttonEncoderL) == LOW){ // dopóki będzie on wciśnięty, 
      if (digitalRead(buttonEncoderR) == LOW) { // sprawdzaj, czy nie jest jednocześnie naciśnięty przycisk prawego enkodera.
        ESP.restart(); // Jeżeli naciśniete sa jednocześnie przyciski obu enkoderów, restartuj radio.
      }
      vTaskDelay(1); // Opóźnienie 1ms zapobiegająca ew. błądom watchdog, skutkującym nieplanowanymi restarami
    }
    g_Mute = !g_Mute; // zmienna informująca o trybie Mute (g_Mute=true -> Mute aktywny, g_Mute=false -> Mute nieaktywny)
                      // zmienna ta przyjmuje wartość przeciwną (tj. false->tru i true->false) za każdym przyciśnięciem lewego enkodera,
                      // co powoduje, że włączamy lub wyłączamy tryb Mute za każdym przyciśnięciem lewego enkodera
    if (g_Mute) { // Jeżeli aktywny jest tryb Mute, to
      audio.setVolume(0); // ustawiamy głośność radia na zero.
      tft.setTextColor(TFT_WHITE, TFT_RED); // Zmieniamy kolor czcionki na biały na czerwonym tle
    }
    else { // Jeżeli wychodzimy z trybu Mute, to
      audio.setVolume(g_Volume); // ustawiamy głośność radia na aktualną wartośćzapisaną w zmiennej g_Volume
      tft.setTextColor(VOLUME_KOLOR, TFT_BLACK); // zmieniamy kolor czcionki na VOLUME_KOLOR na czarnym tle
    }
    tft.setTextDatum(BL_DATUM);
    char cVol[7];
    snprintf(cVol, sizeof(cVol), "Vol:%02d", g_Volume); // Format wyświetlania "%02d" powoduje, że zawsze wyświetlana jest liczba 2-cyfrowa, np. 01,
                                                    // dzięki czemu przy przejściu z liczby 10 na 9 nie pozostanie stara nadmiarowa cyfra 0.
    tft.drawString(cVol, 0, 240); // Wyświetlamy aktualną wartość głośności na pozycji (38, 240);
                                   // fakt, że jesteśmy w trybie Mute, sygnalizowane jest czerwonym tłem czcionki.
  }



  // Prawy enkoder
  if (digitalRead(buttonEncoderR) == LOW) { // Jeżeli przycisk prawego enkodera jest wciśnięty, to...
    if (g_trybPracyRadia == MODE_SCREENSAVER) { // Jeżeli jesteśmy w trybie wygaszacza ekranu, to   
      tft.fillScreen(TFT_BLACK); // Czyścimy ekran - kolor czarny
    }
    
    while (digitalRead(buttonEncoderR) == LOW){ // dopóki będzie on wciśnięty, 
      if (digitalRead(buttonEncoderL) == LOW) { // sprawdzaj, czy nie jest jednocześnie naciśnięty przycisk lewego enkodera.
        ESP.restart(); // Jeżeli naciśniete sa jednocześnie przyciski obu enkoderów, restartuj radio.
      }
      vTaskDelay(1); // Opóźnienie 1ms zapobiegająca ew. błądom watchdog, skutkującym nieplanowanymi restarami
    }
    
    
    if (g_trybPracyRadia == MODE_MAIN || g_trybPracyRadia == MODE_SCREENSAVER) { // Jeżeli nie jesteśmy w trybie Analizatora Widma, to
      if (g_trybPracyRadia == MODE_MAIN) tft.fillScreen(TFT_BLACK); // Czyścimy ekran - kolor czarny
      g_trybPracyRadia = MODE_SPECTRUM_ANALYZER_1; // przechodzimy do trybu Analizatora Widma
      drawSpectrum16(nullptr, 0, 0, 0, 0, 0.0f, 0.0f, true/*Reset*/);
    }
    else if (g_trybPracyRadia == MODE_SPECTRUM_ANALYZER_1) { // Jeżeli jesteśmy w trybie Analizatora Widma, to
      g_trybPracyRadia = MODE_SPECTRUM_ANALYZER_2; // przechodzimy do trybu głównego ekranu
      tft.fillScreen(TFT_BLACK); // Czyścimy ekran - kolor czarny
      drawSpectrum16Invert(nullptr, 0, 0, 0, 0, 0.0f, 0.0f, true/*Reset*/);
    }
    else if (g_trybPracyRadia == MODE_SPECTRUM_ANALYZER_2) { // Jeżeli jesteśmy w trybie Analizatora Widma, to
      g_trybPracyRadia = MODE_MAIN; // przechodzimy do trybu głównego ekranu
      powrotDoGlownegoEkranu(); // i wyświetlamy niezbędne elementy głównego ekranu
    }
    g_czasStartWygaszaczTimeout = millis(); // Aktualizujemy zmienną przechowującą moment, w którym rozpoczęło się odliczanie czasu do aktywacji wygaszacza
  }
  //------------------ B U T T O N S --------------------//
  
  


  // --------- E N K O D E R + G Ł O Ś N O Ś Ć -----------//
  unsigned char result_L = EncL.process(); // sprawdzamy stan lewego enkodera
  if (result_L != DIR_NONE) { // Jeżeli przekręciliśmy lewy enkoder w dowolną stronę 
      if (g_trybPracyRadia == MODE_SCREENSAVER) { // i nie jesteśmy w trybie głównego ekranu, to
        g_trybPracyRadia = MODE_MAIN; // przechodzimy do trybu głównego ekranu
        powrotDoGlownegoEkranu(); // i wyświetlamy niezbędne elementy głównego ekranu
      }
      g_czasStartWygaszaczTimeout = millis(); // Aktualizujemy zmienną przechowującą moment, w którym rozpoczęło się odliczanie czasu do aktywacji wygaszacza
    }  
  if (!g_Mute) {
    if (result_L == DIR_CW && g_Volume < VOLUME_MAX) { // Jeżeli obrót zgodnie z ruchem wskazówek zegara i aktualna głośność jest mniejsza niż wartość maksymalna, to
      g_Volume++; // zwiększ Volume o 1
      g_aktywnaZmianaVolume = true; //Jesteśmy w trakcie zmiany głośności
      g_ostatniaZmianaVolume = millis(); // Aktualizujemy zmienną przechowującą czas ostatniej zmiany głośności
      audio.setVolume(g_Volume); // Ustawiamy wartość g_Volume jako aktualną głośność radia
      if (g_trybPracyRadia == MODE_MAIN) {
        tft.setTextColor(VOLUME_KOLOR, TFT_BLACK); // Zmieniamy kolor czcionki na VOLUME_KOLOR na czarnym tle, dzięki czemu poprzednia wartość będzie wymazywana
        char cVol[7];
        snprintf(cVol, sizeof(cVol), "Vol:%02d", g_Volume); // Format wyświetlania "%02d" powoduje, że zawsze wyświetlana jest liczba 2-cyfrowa, np. 01,
                                                            // dzięki czemu przy przejściu z liczby 10 na 9 nie pozostanie stara nadmiarowa cyfra 0.
        tft.setTextDatum(BL_DATUM);
        tft.drawString(cVol, 0, 240); // Wyświetlamy aktualną wartość głośności na pozycji (38, 240);
      }
      Serial.printf("Volume: %d\n", g_Volume); // wyświetlamy aktualną głośność na Serial Monitorze
    }
    else if (result_L == DIR_CCW && g_Volume > VOLUME_MIN) { // Jeżeli obrót przeciwnie do ruchu wskazówek zegara i aktualna głośność jest większa niż wartość minimalna, to
      g_Volume--; // zmniejsz Volume o 1
      g_aktywnaZmianaVolume = true; //Jesteśmy w trakcie zmiany głośności
      g_ostatniaZmianaVolume = millis(); // Aktualizujemy zmienną przechowującą czas ostatniej zmiany głośności
      audio.setVolume(g_Volume); // Ustawiamy wartość g_Volume jako aktualną głośność radia
      if (g_trybPracyRadia == MODE_MAIN) {
        tft.setTextColor(VOLUME_KOLOR, TFT_BLACK); // Zmieniamy kolor czcionki na VOLUME_KOLOR na czarnym tle, dzięki czemu poprzednia wartość będzie wymazywana
        char cVol[7];
        snprintf(cVol, sizeof(cVol), "Vol:%02d", g_Volume); // Format wyświetlania "%02d" powoduje, że zawsze wyświetlana jest liczba 2-cyfrowa, np. 01,
                                                            // dzięki czemu przy przejściu z liczby 10 na 9 nie pozostanie stara nadmiarowa cyfra 0.
        tft.setTextDatum(BL_DATUM);
        tft.drawString(cVol, 0, 240); // Wyświetlamy aktualną wartość głośności na pozycji (38, 240);
      }
      Serial.printf("Volume: %d\n", g_Volume); // wyświetlamy aktualną głośność na Serial Monitorze
    }
  }
  
  if ((millis() - g_ostatniaZmianaVolume >= 500) && g_aktywnaZmianaVolume) {
    g_aktywnaZmianaVolume = false;
    // -------- P R E F E R E N C E S --------//
    preferences.putUChar("volume", g_Volume); // zapis aktualnej głośności do pamięci nieulotnej Flash
    Serial.printf("Głośność %d zapisana w pamięci Flash\n", g_Volume); // Komunikat o zapisie głośności wyświetlony w Serial.Monitorze
    // -------- P R E F E R E N C E S --------//
  }
  // --------- E N K O D E R + G Ł O Ś N O Ś Ć -----------//



  // ------------------- S T A C J E ---------------------//
  unsigned char result_R = EncR.process(); // Sprawdzamy stan prawego enkodera
  if (result_R != DIR_NONE) { // Jeżeli przekręciliśmy prawy enkoder w dowolną stronę 
    if (g_trybPracyRadia != MODE_MAIN) { // i nie jesteśmy w trybie głównego ekranu, to
      g_trybPracyRadia = MODE_MAIN; // przechodzimy do trybu głównego ekranu
      powrotDoGlownegoEkranu();// i wyświetlamy niezbędne elementy głównego ekranu
    }
    g_czasStartWygaszaczTimeout = millis(); // Aktualizujemy zmienną przechowującą moment, w którym rozpoczęło się odliczanie czasu do aktywacji wygaszacza
  }
  if (result_R == DIR_CW && g_Stacja < 4) { // Jeżeli obrót zgodnie z ruchem wskazówek zegara i jeżeli aktualny indeks stacji jest mniejszy niż maksymalny indeks stacji - u nas 4, to
    g_Stacja++; // zwiększ indeks stacji (g_Stacja) o 1
    tft.setTextDatum(TL_DATUM); // Ustawiamy punkt kotwiczenia tekstu (górny lewy wierzchołek)
    tft.setTextColor(TFT_WHITE, TFT_BLACK); // Ustaw kolor czcionki - biały na czarnym tle - do czyszczenia nazwy stacji i streamu
    tft.fillRect(0, 33, 155, 67, TFT_BLACK); // Czyścimy obszar nazwy stacji i parametrów streamu
    tft.fillRect(0, 105, 320, 75, TFT_BLACK); // Czyścimy obszar tytułu streamu
    tft.setTextColor(TFT_MAGENTA); tft.drawString(Lista_Stacji[g_Stacja][0], 0, STACJE_POS_Y); // Zmieniamy kolor czcionki na magenta i wyświetlamy nazwę ustawionej stacji
                                                                                               // na pozycji (0, STACJE_POS_Y).
    wyswietlLogoStacji(Lista_Stacji[g_Stacja][0]); // Wyświetl logo wybranej stacji
    Serial.printf("Wybrana stacja: %s\n", Lista_Stacji[g_Stacja][0]); // Wyświetl nazwę wybranej stacji w Serial Monitorze
    tft.fillRect(VU_POS_X - 10, VU_POS_Y - 18, 320 - (VU_POS_X - 10), 18, TFT_BLACK); // Czyścimy wskaźnik VU
    audio.stopSong(); // Wstrzymaj odtwarzanie aktualnej
    g_ostatniaZmianaStacji = millis(); // Aktualizujemy zmienną przechowującą czas ostatniej zmiany stacji
    g_aktywnaZmianaStacji = true; //Jesteśmy w trakcie zmiany stacji
    g_streamGotoweParametry = false;
  }
  else if (result_R == DIR_CCW && g_Stacja > 0) { // Jeżeli obrót przeciwnie do ruchu wskazówek zegara i aktualny indeks stacji jest większy od indeksu 0, czyli aktualna stacja nie jest pierwszą stacją na liście, to
    g_Stacja--; // zmniejsz indeks stacji g_Stacja o 1
    tft.setTextDatum(TL_DATUM); // Ustawiamy punkt kotwiczenia tekstu (górny lewy wierzchołek)
    tft.setTextColor(TFT_WHITE, TFT_BLACK); // Ustaw kolor czcionki - biały na czarnym tle - do czyszczenia nazwy stacji i streamu
    tft.fillRect(0, 33, 155, 67, TFT_BLACK); // Czyścimy obszar nazwy stacji i parametrów streamu
    tft.fillRect(0, 105, 320, 75, TFT_BLACK); // Czyścimy obszar tytułu streamu
    tft.setTextColor(TFT_MAGENTA); tft.drawString(Lista_Stacji[g_Stacja][0], 0, STACJE_POS_Y); // Zmieniamy kolor czcionki na magenta i wyświetlamy nazwę ustawionej stacji
                                                                                               // na pozycji (0, STACJE_POS_Y).
    wyswietlLogoStacji(Lista_Stacji[g_Stacja][0]); // Wyświetl logo wybranej stacji
    Serial.printf("Wybrana stacja: %s\n", Lista_Stacji[g_Stacja][0]); // Wyświetl nazwę wybranej stacji w Serial Monitorze
    tft.fillRect(VU_POS_X - 10, VU_POS_Y - 18, 320 - (VU_POS_X - 10), 18, TFT_BLACK); // Czyścimy wskaźnik VU
    audio.stopSong(); // Wstrzymaj odtwarzanie aktualnej
    g_ostatniaZmianaStacji = millis(); // Aktualizujemy zmienną przechowującą czas ostatniej zmiany stacji
    g_aktywnaZmianaStacji = true; //Jesteśmy w trakcie zmiany stacji
    g_streamGotoweParametry = false;
  }
  

  if ((millis() - g_ostatniaZmianaStacji >= 500) && g_aktywnaZmianaStacji) {
    g_aktywnaZmianaStacji = false;
    // -------- P R E F E R E N C E S --------//
    preferences.putUChar("stacja", g_Stacja); // zapis aktualnie wybranej stacji do pamięci nieulotnej Flash
    Serial.printf("Stacja %d zapisana w pamięci Flash\n", g_Stacja); // Komunikat o zapisie stacji wyświetlony w Serial.Monitorze
    // -------- P R E F E R E N C E S --------//
    audio.connecttohost(Lista_Stacji[g_Stacja][1]); // Połącz ze stacją o ustawionym indeksie g_Stacja
  }
  // ------------------- S T A C J E ---------------------//


  audio.loop(); // Główna pętla odtwarzacza

  
  // ----------------------- V U -------------------------//
  if (g_trybPracyRadia == MODE_MAIN && (millis() - g_czasOstatniegoNarysowaniaVU >= 50) && !g_aktywnaZmianaStacji) { // Wskaźnik VU będziemy wyświetlali co 50ms, ale nie podczas zmiany stacji;
    drawVU(VU_POS_X, VU_POS_Y); // Rysujemy wskaźnik VU na pozycji (VU_POS_X, VU_POS_Y) <- współrzędne lewego dolnego rogu wskaźnika
    g_czasOstatniegoNarysowaniaVU = millis(); // Aktualizujemy zmienną przechowującą czas ostatniego narysowania wskaźnika
  }
  // ----------------------- V U -------------------------//

  // --------------------- W i F i -----------------------//
  if (g_trybPracyRadia == MODE_MAIN && (millis() - g_czasOstatniegoStatusuWiFi >= 1000)) { // Status WiFi będziemy wyświetlali co 1s;  
    wyswietlStatusWiFi(); // Wyświetlamy status WiFi
    g_czasOstatniegoStatusuWiFi = millis(); // Aktualizujemy zmienną przechowującą czas ostatniego wyświetlenia statusu WiFi
  }  
  // --------------------- W i F i -----------------------//



  // ---------------- W Y G A S Z A C Z -------------------//
  if (g_trybPracyRadia == MODE_MAIN && (millis() - g_czasStartWygaszaczTimeout >= WYGASZACZ_TIMEOUT)) { // Przejście do trybu wygaszacza po upływie (WYGASZACZ_TIMEOUT/1000) sekund bezczynności 
    g_trybPracyRadia = MODE_SCREENSAVER; // Ustawiamy radio w tryb wygaszacza ekranu
    g_imageNumber = 0; 
    wyswietlObraz(); // Wyswietlamy pierwszy obraz (bo zmienna g_imageNumber = 0, ale w funkcji wyswietlObraz() zwiększamy ją o 1 przed wyświetleniem obrazu)
    g_czasStartWygaszaczImage = millis(); // Aktualizujemy zmienną przechowującą czas wyświetlenia ostatniego obrazu wygaszacza ekranu
  }  

  if (g_trybPracyRadia == MODE_SCREENSAVER && (millis() - g_czasStartWygaszaczImage >= WYGASZACZ_IMAGE_DURATION)) { // Wyświetlenie kolejnego obrazu po upływie (WYGASZACZ_IMAGE_DURATION/1000) sekund w trybie wygaszacza ekranu
    wyswietlObraz(); // Wyświetlamy kolejne obrazy
    g_czasStartWygaszaczImage = millis(); // Aktualizujemy zmienną przechowującą czas wyświetlenia ostatniego obrazka wygaszacza ekranu
  }
  // ---------------- W Y G A S Z A C Z -------------------//



  // --------- A N A L I Z A T O R   W I D M A ------------//
  if ( (g_trybPracyRadia == MODE_SPECTRUM_ANALYZER_1 || g_trybPracyRadia == MODE_SPECTRUM_ANALYZER_2) && (millis() - g_czasStartWyswietlAnalizatorWidma >= DISPLAY_ANALYZER_PERIOD) ) { // Wyświetlenie analizatora widma co DISPLAY_ANALYZER_PERIOD sekund
    wyswietlAnalizatorWidma();
    g_czasStartWyswietlAnalizatorWidma = millis(); // Aktualizujemy zmienną przechowującą czas ostatniego wyświetlenia analizatora widma
  }
  // --------- A N A L I Z A T O R   W I D M A ------------//



  vTaskDelay(1); // Opóźnienie 1ms zapobiegające ew. błędom Watchdog Error
}





/********************************************************************************/
/*                                                                              */
/*                    A N A L I Z A T O R Y   W I D M A                         */
/*                                                                              */
/********************************************************************************/

// Funkcja rysuje 16-segmentowy analizator widma
void drawSpectrum16(float *spectrum, int xl, int yl, int width, int height, float colGapPercent, float segGapPercent, bool reset) {
    constexpr int bands = 16;
    const int bins = SIZE_OF_FFT / 2;
    const int bandBins = (bins / bands > 0) ? (bins / bands) : 1; 

    static float peak[bands];
    static float smoothHeight[bands];
    static int lastSegs[bands];
    static float peakPixel[bands];
    static int lastPeakPixel[bands];
    static unsigned long peakHoldTime[bands];
    static bool initialized = false;

    const float gain[bands] = {1.2f, 1.2f, 1.3f, 1.3f, 1.5f, 1.5f, 1.5f, 1.5f, 1.5f, 1.5f, 1.5f, 1.5f, 1.5f, 1.4f, 1.4f, 1.4f};
    const float decay = 0.97f;
    const int peakFallSpeed = 2; 
    const int maxSegs = 16;
    const unsigned long holdDuration = 600; 
    const uint16_t peakColor = tft.color565(180, 180, 180); 
    const uint8_t normalSegmentsCount = 12;
    const uint8_t mediumSegmentsCount = 15;

    auto resetBuffers = [&]() {
        for (int i = 0; i < bands; i++) {
            peak[i] = 1.0f;
            smoothHeight[i] = 0.0f;
            lastSegs[i] = 0;
            peakPixel[i] = 0.0f;
            lastPeakPixel[i] = -1;
            peakHoldTime[i] = 0;
        }
        initialized = true;
    };

    if (reset) {
        resetBuffers();
        return;
    }
    if (!initialized) resetBuffers();

    int rawBarWidth = width / bands;
    int gap = (int)(rawBarWidth * colGapPercent);
    int barWidth = (width - (bands - 1) * gap) / bands;
    int segHeight = height / (maxSegs + (maxSegs - 1) * segGapPercent);
    int segGap = (int)(segHeight * segGapPercent);
    int peakHeightPx = segHeight / 4;
    if (peakHeightPx <= 1) peakHeightPx = 2;

    for (int b = 0; b < bands; b++) {
        float sum = 0.0f;
        int start = b * bandBins;
        int end   = (b + 1) * bandBins;
        for (int i = start; i < end; i++) sum += spectrum[i];
        float avg = sum / bandBins;
        if (avg > peak[b]) peak[b] = avg;
        else peak[b] *= decay;
        float norm = (avg * gain[b]) / (peak[b] > 0.001f ? peak[b] : 1.0f);
        if (norm > 1.0f) norm = 1.0f;
        int newSegs = (int)(norm * maxSegs);
        smoothHeight[b] = SMOOTHING_FACTOR_S16 * (float)newSegs + (1.0f - SMOOTHING_FACTOR_S16) * smoothHeight[b];
        int smoothSegs = (int)(smoothHeight[b]);
        int oldSegs = lastSegs[b];
        lastSegs[b] = smoothSegs;
        int x = xl + b * (barWidth + gap);
        int currentHeightPx = smoothSegs * (segHeight + segGap);

        // Peak Holdery - czyszczenie
        if ((float)currentHeightPx >= peakPixel[b]) {
            peakPixel[b] = (float)currentHeightPx;
            peakHoldTime[b] = millis();
        } else {
            if (millis() - peakHoldTime[b] > holdDuration) {
                peakPixel[b] -= (float)peakFallSpeed;
                if (peakPixel[b] < 0) peakPixel[b] = 0;
            }
        }
        int yPeak = yl - (int)peakPixel[b] - peakHeightPx;
        if (lastPeakPixel[b] != -1 && lastPeakPixel[b] != yPeak) {
            tft.fillRect(x, lastPeakPixel[b], barWidth, peakHeightPx, TFT_BLACK);
        }

        // Segmenty
        for (int s = 0; s < maxSegs; s++) {
            bool wasActive = s < oldSegs;
            bool isActive  = s < smoothSegs;
            int segY = yl - (s + 1) * (segHeight + segGap) + segGap;
            bool peakWasHere = (lastPeakPixel[b] != -1) && 
                               (lastPeakPixel[b] >= segY) && 
                               (lastPeakPixel[b] < segY + segHeight);
            if ((isActive != wasActive) || (isActive && peakWasHere)) {
                uint16_t color;
                if (isActive) {
                    if (s < normalSegmentsCount) color = TFT_GREEN;
                    else if (s < mediumSegmentsCount) color = TFT_ORANGE;
                    else color = TFT_RED;
                } else {
                    color = TFT_BLACK;
                }
                tft.fillRoundRect(x, segY, barWidth, segHeight, 2, color);
            }
        }

        // Peak Holdery - rysowanie
        if (yPeak < yl - 2 && peakPixel[b] > 0) {
            if (yPeak != lastPeakPixel[b]) {
                tft.fillRect(x, yPeak, barWidth, peakHeightPx, peakColor);
                lastPeakPixel[b] = yPeak;
            }
        } else {
            if (lastPeakPixel[b] != -1) {
                tft.fillRect(x, lastPeakPixel[b], barWidth, peakHeightPx, TFT_BLACK);
                lastPeakPixel[b] = -1;
                peakPixel[b] = 0;
            }
        }
    }
}





// Funkcja rysuje odwrócony 16-segmentowy analizator widma
void drawSpectrum16Invert(float *spectrum, int xl, int yl, int width, int height, float colGapPercent, float segGapPercent, bool reset) {
    constexpr int bands = 16;
    const int bins = SIZE_OF_FFT / 2;
    const int bandBins = (bins / bands > 0) ? (bins / bands) : 1; 

    static float peak[bands];
    static float smoothHeight[bands];
    static int lastSegs[bands];
    static float peakPixel[bands];
    static int lastPeakPixel[bands];
    static unsigned long peakHoldTime[bands];
    static bool initialized = false;

    const float gain[bands] = {1.2f, 1.2f, 1.3f, 1.3f, 1.5f, 1.5f, 1.5f, 1.5f, 1.5f, 1.5f, 1.5f, 1.5f, 1.5f, 1.4f, 1.4f, 1.4f};
    const float decay = 0.97f;
    const int peakFallSpeed = 2; 
    const int maxSegs = 16;
    const unsigned long holdDuration = 600; // 0,6s wstrzymania
    const int maxPeakBrightness = 210;      // Nowa jasność startowa (lekka szarość zamiast czystej bieli)

    auto resetBuffers = [&]() {
        for (int i = 0; i < bands; i++) {
            peak[i] = 1.0f;
            smoothHeight[i] = 0.0f;
            lastSegs[i] = 0;
            peakPixel[i] = 0.0f;
            lastPeakPixel[i] = -1;
            peakHoldTime[i] = 0;
        }
        initialized = true;
    };

    if (reset) {
        resetBuffers();
        return;
    }
    if (!initialized) resetBuffers();

    int rawBarWidth = width / bands;
    int gap = (int)(rawBarWidth * colGapPercent);
    int barWidth = (width - (bands - 1) * gap) / bands;
    int segHeight = height / (maxSegs + (maxSegs - 1) * segGapPercent);
    int segGap = (int)(segHeight * segGapPercent);
    int peakHeightPx = segHeight / 4;
    if (peakHeightPx <= 1) peakHeightPx = 2;

    for (int b = 0; b < bands; b++) {
        uint16_t barColor;
        if (b < 4) { 
            uint8_t g = map(b, 0, 3, 0, 255);
            barColor = tft.color565(0, g, 255);
        } else if (b < 9) { 
            uint8_t r = map(b, 4, 8, 0, 255);
            uint8_t bl = map(b, 4, 8, 255, 0);
            barColor = tft.color565(r, 255, bl);
        } else { 
            uint8_t g = map(b, 9, 15, 200, 0); 
            barColor = tft.color565(255, g, 0);
        }
        float sum = 0.0f;
        int start = b * bandBins;
        int end   = (b + 1) * bandBins;
        for (int i = start; i < end; i++) sum += spectrum[i];
        float avg = sum / bandBins;
        if (avg > peak[b]) peak[b] = avg;
        else peak[b] *= decay;
        float norm = (avg * gain[b]) / (peak[b] > 0.001f ? peak[b] : 1.0f);
        if (norm > 1.0f) norm = 1.0f;
        int newSegs = (int)(norm * maxSegs);
        smoothHeight[b] = SMOOTHING_FACTOR_S16 * (float)newSegs + (1.0f - SMOOTHING_FACTOR_S16) * smoothHeight[b];
        int smoothSegs = (int)(smoothHeight[b]);
        int oldSegs = lastSegs[b];
        lastSegs[b] = smoothSegs;
        int x = xl + b * (barWidth + gap);
        int currentHeightPx = smoothSegs * (segHeight + segGap);
        if ((float)currentHeightPx >= peakPixel[b]) {
            peakPixel[b] = (float)currentHeightPx;
            peakHoldTime[b] = millis();
        } else {
            if (millis() - peakHoldTime[b] > holdDuration) {
                peakPixel[b] += (float)peakFallSpeed;
            }
        }
        int yPeak = yl + (int)peakPixel[b];
        int brightness = 0;
        if (yPeak < 240 - peakHeightPx) {
            int startFadeY = yl + currentHeightPx;
            if (yPeak <= startFadeY) {
                brightness = maxPeakBrightness; 
            } else {
                brightness = map(yPeak, startFadeY, 240, maxPeakBrightness, 0);
            }
            brightness = constrain(brightness, 0, maxPeakBrightness);
        }
        uint16_t peakColor = tft.color565(brightness, brightness, brightness);

        // Peak Holdery - czyszczenie
        if (lastPeakPixel[b] != -1 && lastPeakPixel[b] != yPeak) {
            tft.fillRect(x, lastPeakPixel[b], barWidth, peakHeightPx, TFT_BLACK);
        }

        // Segmenty
        for (int s = 0; s < maxSegs; s++) {
            bool wasActive = s < oldSegs;
            bool isActive  = s < smoothSegs;
            bool peakWasHere = (lastPeakPixel[b] != -1) && 
                               (lastPeakPixel[b] >= yl + s * (segHeight + segGap)) && 
                               (lastPeakPixel[b] < yl + (s + 1) * (segHeight + segGap));
            if ((isActive != wasActive) || (isActive && peakWasHere)) {
                int yTop = yl + s * (segHeight + segGap);
                uint16_t color = isActive ? barColor : TFT_BLACK;
                tft.fillRoundRect(x, yTop, barWidth, segHeight, 2, color);
            }
        }

        // Peak Holdery - rysowanie
        if (yPeak < 240 - peakHeightPx && brightness > 5) {
            tft.fillRect(x, yPeak, barWidth, peakHeightPx, peakColor);
            lastPeakPixel[b] = yPeak;
        } else {
            if (lastPeakPixel[b] != -1) {
                tft.fillRect(x, lastPeakPixel[b], barWidth, peakHeightPx, TFT_BLACK);
                lastPeakPixel[b] = -1;
                peakPixel[b] = 0;
            }
        }
    }
}

