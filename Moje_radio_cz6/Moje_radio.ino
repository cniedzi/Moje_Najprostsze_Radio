// W Arduino IDE musimy zainstalować biblioteki:
// 1. ESP32-audioI2S
// 2. Adafruit GFX Library
// 3. Adafruit ST7735 and ST7789 lub Adafruit ILI9341 - bibliotekę dobieramy do naszego wyświetlacza
#include <WiFi.h>
#include "Audio.h" // (https://github.com/schreibfaul1/ESP32-audioI2S)
#include "Adafruit_GFX.h"
#include <Adafruit_ST7789.h>  // dołączamy bibliotekę obsługującą wyświetlacz ST7789
#include <Adafruit_ILI9341.h> // dołączamy bibliotekę obsługującą wyświetlacz ILI9341

// --------------------- L O G O -----------------------//
#include "Logo.h"
// --------------------- L O G O -----------------------//

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

// Definiujemy kolory
#define TFT_WHITE 0xFFFF
#define TFT_RED 0xF800
#define TFT_MAGENTA 0xF81F
#define TFT_ORANGE 0xFD20
#define TFT_CYAN 0x07FF
#define TFT_GREEN 0x07E0
#define TFT_BLACK 0x0000

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
#define VOLUME_KOLOR 0xf492 // https://rgbcolorpicker.com/565 <- tytaj można sprawdzać kody doowolnych kolorów RGB565 (3 pole tekstowe)
// ----------------- G Ł O Ś N O Ś Ć -------------------//

// ----------------------- V U -------------------------//
#define VU_POS_X 160 // Współrzędna X lewego dolnego rogu wskaźnika VU
#define VU_POS_Y 239 // Współrzędna Y lewego dolnego rogu wskaźnika VU
// ----------------------- V U -------------------------//


// --------------------- W i F i -----------------------//
#define KOLOR_WIFI 0xBDD7 // Definicja koloru do wyświetlania parametrów WiFi
#define WIFI_POS_Y 195 // Definicja współrzędnej Y (w pionie) parametrów WiFi 
// --------------------- W i F i -----------------------//


// Definicja niezbędnych obiektów
SPIClass spi(HSPI); // Interfejs SPI
Adafruit_ST7789 tft = Adafruit_ST7789(&spi, TFT_CS, TFT_DC, TFT_RST); // Ta linia powinna być aktywna dla wyswietlacza ST7789
//Adafruit_ILI9341 tft = Adafruit_ILI9341(&spi, TFT_DC, TFT_CS, TFT_RST); // Ta linia powinna być aktywna dla wyswietlacza ILI9341
Audio audio; // Odtwarzacz audio

// ------------------ E N K O D E R --------------------//
Rotary EncL = Rotary(S1_EncoderL, S2_EncoderL); // Lewy enkoder do regulacji głośności radia
Rotary EncR = Rotary(S1_EncoderR, S2_EncoderR); // Prawy enkoder do wyboru stacji
// ------------------ E N K O D E R --------------------//


// ------------------- S T A C J E ---------------------//
#define STACJE_POS_Y 45
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
// ----------------- G Ł O Ś N O Ś Ć -------------------//

// ----------------------- V U -------------------------//
unsigned long g_czasOstatniegoNarysowaniaVU = millis(); // Zmienna, w której będziemy przechowywali czas ostatniego narysowania wskaźnika VU. Funkcja millis() zwraca liczbę milisekund, które upłynęły od momentu włączenia radia do chwili wywołania tej funkcji
// ----------------------- V U -------------------------//

// --------------------- W i F i -----------------------//
unsigned long g_czasOstatniegoStatusuWiFi = millis(); // Zmienna, w której będziemy przechowywali czas ostatniego wyświetlenia statusu WiFi
// --------------------- W i F i -----------------------//


// Opcjonalna funkcja wyświetlająca komunikaty odtwarzacza audio w Serial Monitorze oraz nazwę streamu na wyświetlaczu
void my_audio_info(Audio::msg_t m) {
  Serial.printf("%s: %s\n", m.s, m.msg);
  if (m.e == Audio::evt_streamtitle) {
    String tytulStreamu = String(m.msg);
    if (tytulStreamu.length() > 78) tytulStreamu = tytulStreamu.substring(0, 78); // ograniczamy tytuł streamu do maksymalnie 3 linii (3x26znaków = 78 znaków).
                                                                                  // W rzeczywistości jeżeli nazwa streamu będzie zawierała polskie znaki,
                                                                                  // to tekst będzie krótszy (bo 1 znak PL to 2 znaki w stringu), ale na potrzeby tego ćwiczenia nie przejmujemy się tym :)
    tft.setTextSize(2); // Ustawiamy rozmiar tekstu na 2
    tft.setTextColor(TFT_CYAN, TFT_BLACK); // Ustawiamy kolor tekstu na cyjanowy na czarnym tle
    tft.setCursor(0, 130); // Ustawiamy kursor na pozycji (0,130)
    tft.print("                                                                                                        "); // Czyścimy poprzedni tytuł streamu
    tft.setCursor(0, 130); // Jeszcze raz ustawiamy kursor na pozycji (0,130)...
    tft.print(m.msg); //i wyświetlamy nowy tytuł streamu
  }
  else if (m.e == Audio::evt_info) {
    String info(m.msg);
    if (String(info).indexOf("BitsPerSample:") != -1) g_streamGotoweParametry = true; // Pojawiły się nowe parametry streamu, więc możemy je wyświetlić (g_streamGotoweParametry = true) 
  }
}


// --------------------- L O G O -----------------------//
// Ta funkcja będzie wyświetlała logo stacji o nazwie nazwaStacji przekazanej jako parametr w wywołaniu funkcji
void wyswietlLogoStacji(String nazwaStacji) { // do funkcji przekazujemy nazwę stacji, dla której chcemy wyświetlić logo
  // Przyjmujemy następujące współrzędne środka logo:
  // x_sr = 230
  // y_sr = 75
  tft.fillRect(170, 45, 120, 60, TFT_BLACK); // Czyścimy poprzednie logo przed wyświetleniem nowego
  if (nazwaStacji == "ITALO4YOU") tft.drawRGBBitmap(200, 45, ITALO4YOU, 60, 60);
  else if (nazwaStacji == "ESKA") tft.drawRGBBitmap(170, 55, ESKA, 120, 40);
  else if (nazwaStacji == "RMF FM") tft.drawRGBBitmap(183, 45, RMFFM, 95, 60);
  else if (nazwaStacji == "VOX FM") tft.drawRGBBitmap(200, 45, VOXFM, 60, 60);
  else if (nazwaStacji == "ESKA ROCK") tft.drawRGBBitmap(195, 45, ESKAROCK, 70, 60);
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

  tft.setTextSize(1); // Ustawiamy rozmiar czcionki na 1
  tft.setTextColor(TFT_WHITE); // Ustawiamy kolor czcionki na biały
  tft.setCursor(x - 10, y - gap - 16 + 1); tft.print("L"); // Przed paskiem lewego kanału wyświetlamy literę "L"
  tft.setCursor(x - 10, y - 8 + 1); tft.print("P"); // Przed paskiem prawego kanału wyświetlamy literę "P"

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
  tft.setTextSize(2); // Ustawiamy rozmiar textu 2
  tft.setTextColor(KOLOR_WIFI, TFT_BLACK); // Ustawiamy kolor tekstu na zdefiniowany KOLOR_WIFI na czarnym tle
  tft.setCursor(0, WIFI_POS_Y); // Ustawiamy kursor na pozycji (0, WIFI_POS_Y)
  tft.printf("%3ddBm", poziomWiFi); // wyświetlamy aktualny poziom sygnału WiFi

  uint16_t kolorWskaznikaSygnalu;
  if (poziomWiFi <= -85) kolorWskaznikaSygnalu = TFT_RED; // jeżeli poziom jest niższy niż -85dBm, to wskaźnik rysujemy na czerwono
  else if (poziomWiFi <= -75) kolorWskaznikaSygnalu = TFT_ORANGE; // jeżeli poziom jest wyższy niż -85dBm, ale niższy niż -75 dbM, to wskaźnik rysujemy na pomarańczowo
  else kolorWskaznikaSygnalu = KOLOR_WIFI; // // jeżeli poziom jest wyższy niż -75dBm, to wskaźnik rysujemy zdefiniowanym kolorem KOLOR_WIFI
  const uint16_t x_start = 75; // współrzędna X wskaźnika spoziomu sygnału
  const uint16_t y_base = WIFI_POS_Y + 14; // // współrzędna Y wskaźnika spoziomu sygnału
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
  if (nazwaSieci.length() > 16) nazwaSieci = nazwaSieci.substring(0, 16); // Ograniczamy długość wyświetlanej nazwy sieci do 16 znaków
  tft.setTextSize(2); // Ustawiamy rozmiar czcionki 2
  tft.setTextColor(KOLOR_WIFI); // Ustawiamy kolor czcionki na KOLOR_WIFI
  tft.setCursor(115, WIFI_POS_Y); // Ustawiamy kursor na pozycji (115, WIFI_POS_Y)
  tft.print(nazwaSieci.c_str());  // Wyświetlamy nazw sieci WiFi
}
// --------------------- W i F i -----------------------//





// Funkcja setup
void setup() {
  pinMode(TFT_BL, OUTPUT); // Ustawienie pinu odpowiedzialnego za podświetlenie wyświetlacza jako wyjście
  digitalWrite(TFT_BL, LOW); // Wyłączenie podświetlenia wyświetlacza, żeby uniknać artefaktów podczas inicjalizacji

  // Ustawienie pinów enkoderów jako wejść
  pinMode(S1_EncoderR, INPUT_PULLUP);
  pinMode(S2_EncoderR, INPUT_PULLUP);
  pinMode(S1_EncoderL, INPUT_PULLUP);
  pinMode(S2_EncoderL, INPUT_PULLUP);
  
  Serial.begin(115200); // Inicjalizacja monitora portu szeregowego, tj. Serial Monitor

  //WiFi.begin("nazwa_sieci_wifi", "hasło_sieci_wifi"); // Podłączenie do sieci WiFi -> WAŻNE: trzeba wpisać swoje parametry sieci WiFi
    
  spi.begin(TFT_SCLK, -1, TFT_MOSI, TFT_CS); // Inicjalizacja interfejsu SPI
  
  
  // !!! Poniżej trzeba pozostawić aktywną tylko jedną sekcję, w zależności od posiadanego wyświetlacza !!!
  //
  // ---------- ST7789 ----------//
  tft.init(240, 320); // Inicjalizacja wyświetlacza ST7789 ( dla ILI9341 zamiast tego będzie tft.begin(); )
  tft.invertDisplay(false); // Jeżeli kolory są wyświetlane w negatywie, trzeba odkomentować tę albo następną komendę (tj. usunąć "//" na początku linii)
  tft.invertDisplay(true); // Jeżeli kolory są wyświetlane w negatywie, trzeba odkomentować tę albo poprzednią komendę (tj. usunąć "//" na początku linii)
  tft.setRotation(1); // Dopasowanie obrotu obrazu ( dla ILI9341 zamiast tego będzie tft.setRotation(3); )
  // ---------- ST7789 ----------//
  
  // ---------- ILI9341 ---------//
  //tft.begin();
  //tft.invertDisplay(false); // Jeżeli kolory są wyświetlane w negatywie, trzeba odkomentować tę albo następną komendę (tj. usunąć "//" na początku linii)
  //tft.invertDisplay(true); // Jeżeli kolory są wyświetlane w negatywie, trzeba odkomentować tę albo poprzednią komendę (tj. usunąć "//" na początku linii)
  //tft.setRotation(3);
  // ---------- ILI9341 ---------//
  

  tft.setTextWrap(true); // Tekst dłuższy niż szerokość ekranu będzie zawijany do kolejnej linii
  tft.fillScreen(TFT_BLACK); // Czyścimy ekran - kolor czarny

  // --------------------- L O G O -----------------------//
  // Do wyświetlania bitmap będziemy używać funkcji z biblioteki Adafruit GFX:
  //drawRGBBitmap(x_lewy_górny_wierzchołek_logo, y__lewy_górny_wierzchołek_logo, *bitmapa, szerokosc_bitmapy, wysokosc_bitmapy);
  tft.setTextSize(3); // Ustawiamy rozmiar czcionki wyświetlacza na 3
  tft.setTextColor(TFT_ORANGE); // Ustawiamy kolor czcionki wyświetlacza
  tft.setCursor(61,0); // Ustawiamy pozycję (x, y) tekstu, na której będzie wyswietlony tekst
  tft.print("NAJPROSTSZE"); // Wyswietlamy napis "RADIO INTERNETOWE" na pozycji (0, 0)
  tft.setCursor(7,30); // Ustawiamy pozycję (x, y) tekstu, na której będzie wyswietlony tekst
  tft.print("RADIO INTERNETOWE"); // Wyswietlamy napis "RADIO INTERNETOWE" na pozycji (0, 0)
  tft.drawRGBBitmap(20, 77, LOGO, 280, 141); // Wyświetlamy logo startowe; dobieramy współrzedne lewego górnego wierzchołka tak, żeby logo było wycentrowane
  digitalWrite(TFT_BL, HIGH); // Włączamy podświetlenie wyświetlacza
  delay(5000); // Przez 5 sekund podziwiamy logo :)
  tft.fillScreen(TFT_BLACK); // Czyścimy ekran - kolor czarny
  // --------------------- L O G O -----------------------//

  tft.setTextSize(3); // Ustawiamy rozmiar czcionki wyświetlacza na 3
  tft.setTextColor(TFT_ORANGE); // Ustawiamy kolor czcionki wyświetlacza
  tft.setCursor(0,0); // Ustawiamy pozycję (x, y) tekstu, na której będzie wyswietlony tekst
  tft.print("RADIO INTERNETOWE"); // Wyswietlamy napis "RADIO INTERNETOWE" na pozycji (0, 0)
  tft.setTextSize(2); // Ustawiamy rozmiar czcionki wyświetlacza na 2
  tft.setTextColor(TFT_MAGENTA); tft.setCursor(0, STACJE_POS_Y); tft.print(Lista_Stacji[g_Stacja][0]); // Zmieniamy kolor czcionki na cyjanowy i wyświetlamy zdefiniowaną wcześniej nazwę stacji
                                                                                                                                             // wielkimi literami, na pozycji (0, STACJE_POS_Y)
  
  // --------------------- L O G O -----------------------//
  wyswietlLogoStacji(Lista_Stacji[g_Stacja][0]);
  // --------------------- L O G O -----------------------//
  

  // ----------------- G Ł O Ś N O Ś Ć -------------------//
  tft.setTextColor(VOLUME_KOLOR); tft.setCursor(0, 225); tft.print("Vol:"); // Zmieniamy kolor czcionki na kolor VOLUME_KOLOR i wyświetlamy napis "Volume:" na pozycji (0, 225)
  tft.setCursor(48, 225); tft.printf("%02d", g_Volume); // Wyświetlamy aktualną wartość głośności na pozycji (96, 225);
                                                                                        // format wyświetlania "%02d" powoduje, że zawsze wyświetlana jest liczba 2-cyfrowa, np. 01
  // ----------------- G Ł O Ś N O Ś Ć -------------------//
 
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
  
  // ----------------- G Ł O Ś N O Ś Ć -------------------//
  audio.setVolume(g_Volume); // Ustawiamy głośność dźwięku (standardowy zakres 0...21)
  // ----------------- G Ł O Ś N O Ś Ć -------------------//
  
  audio.connecttohost(Lista_Stacji[g_Stacja][1]); // podłączamy się do stacji, której indeks jest zdefiniowany w zmiennej g_Stacja, czyli teraz 0 (tj. pierwsza stacja),
                                                  // a [1] oznacza, że bierzemy 2 kolumnę z wiersza [g_stacja] (pamiętamy, że indeksy są o 1 mniejsze niż rzeczywiste numery kolumn i wierszy),
                                                  // po czym powinniśmy usłyszeć dźwięk (szczegóły połączenie możemy obserwować na Serial Monitorze)
}


// Główna pętla loop
void loop() {
  // Wyświetlanie parametrów streamu
  if (g_streamGotoweParametry) { // Jeżeli parametry streamu są już dostępne, to
    tft.setTextSize(2); // ustawiamy rozmiar tekstu na 2
    tft.setTextColor(0xf68d, TFT_BLACK); // ustawiamy kolor tekstu na 0xf68d na czarnym tle
    tft.setCursor(0, 80); // ustawiamy kursor na pozycji (0, 80)
    tft.print(audio.getCodecname()); // wyświetlamy nazwę kodeka (tj. MP3, AAC, FLAC, itd.)
    tft.print(" "); // wyświetlamy odstęp
    tft.print(audio.getBitRate()/1000L); // wyświetlamy bitrate streamu (np. 192, 256, etc.)
    tft.print("k"); // za liczbą bitrate, wyświetlamy literę "k" - skrót od kilo
    tft.setCursor(0, 100); // ustawiamy kursor na pozycji (0, 100)
    tft.print(audio.getSampleRate()); // wyświetlamy Sample Rate (np. 44100, 48000, etc.)
    tft.print("Hz"); // na końcu wyświetlamy jednostkę "Hz"
    g_streamGotoweParametry = false; // po wyświetleniu aktualnych parametrów, ustawiamy g_streamGotoweParametry na false (żeby uniknąć ciągłego wyświetlania tych samych parametrów)
  }


  // --------- E N K O D E R + G Ł O Ś N O Ś Ć -----------//
  unsigned char result_L = EncL.process(); // sprawdzamy stan lewego enkodera
  if (result_L == DIR_CW && g_Volume < VOLUME_MAX) { // Jeżeli obrót zgodnie z ruchem wskazówek zegara i aktualna głośność jest mniejsza niż wartość maksymalna, to
    g_Volume++; // zwiększ Volume o 1
    audio.setVolume(g_Volume); // Ustawiamy wartość g_Volume jako aktualną głośność radia
    tft.setTextSize(2); // Ustawiamy rozmiar czcionki wyświetlacza na 2
    tft.setTextColor(VOLUME_KOLOR, TFT_BLACK); tft.setCursor(48, 225); tft.printf("%02d", g_Volume); // Zmieniamy kolor czcionki na VOLUME_KOLOR i wyświetlamy aktualną wartość głośności na pozycji (96, 225);
                                                                                                        // WAŻNE: ustawiliśmy też czarny kolor tła czcionki, dzięki czemu poprzednia wartość będzie wymazywana,
                                                                                                        // a format wyświetlania "%02d" powoduje, że zawsze wyświetlana jest liczba 2-cyfrowa, np. 01,
                                                                                                        // dzięki czemu przy przejściu z liczby 10 na 9 nie pozostanie stara nadmiarowa cyfra 0.
    Serial.printf("Volume: %d\n", g_Volume); // wyświetlamy aktualną głośność na Serial Monitorze
  }
  else if (result_L == DIR_CCW && g_Volume > VOLUME_MIN) { // Jeżeli obrót przeciwnie do ruchu wskazówek zegara i aktualna głośność jest większa niż wartość minimalna, to
    if (g_Volume > VOLUME_MIN) g_Volume--; // zmniejsz Volume o 1
    audio.setVolume(g_Volume); // Ustawiamy wartość g_Volume jako aktualną głośność radia
    tft.setTextSize(2); // Ustawiamy rozmiar czcionki wyświetlacza na 2
    tft.setTextColor(VOLUME_KOLOR, TFT_BLACK); tft.setCursor(48, 225); tft.printf("%02d", g_Volume); // Zmieniamy kolor czcionki na VOLUME_KOLOR i wyświetlamy aktualną wartość głośności na pozycji (96, 225);
                                                                                                        // WAŻNE: ustawiliśmy też czarny kolor tła czcionki, dzięki czemu poprzednia wartość będzie wymazywana,
                                                                                                        // a format wyświetlania "%02d" powoduje, że zawsze wyświetlana jest liczba 2-cyfrowa, np. 01,
                                                                                                        // dzięki czemu przy przejściu z liczby 10 na 9 nie pozostanie stara nadmiarowa cyfra 0.
    Serial.printf("Volume: %d\n", g_Volume); // wyświetlamy aktualną głośność na Serial Monitorze
  }
  // --------- E N K O D E R + G Ł O Ś N O Ś Ć -----------//



  // ------------------- S T A C J E ---------------------//
  unsigned char result_R = EncR.process(); // Sprawdzamy stan prawego enkodera
  if (result_R == DIR_CW && g_Stacja < 4) { // Jeżeli obrót zgodnie z ruchem wskazówek zegara i jeżeli aktualny indeks stacji jest mniejszy niż maksymalny indeks stacji - u nas 4, to
    g_Stacja++; // zwiększ indeks stacji (g_Stacja) o 1
    tft.setTextSize(2); //Ustaw rozmiar czcionki 2
    tft.setTextColor(TFT_WHITE, TFT_BLACK); // Ustaw kolor czcionki - biały na czarnym tle - do czyszczenia nazwy stacji i streamu
    tft.setCursor(0,STACJE_POS_Y); tft.print("            "); // Ustaw kursor na pozycji (0, STACJE_POS_Y) i wyświetl pusty tekst o długości 12 znaków, żeby wyczyścić poprzednią nazwę stacji
    tft.setCursor(0,80); tft.print("            "); // Ustaw kursor na pozycji (0, 80) i wyświetl pusty tekst o długości 12 znaków, żeby wyczyścić parametry streamu
    tft.setCursor(0,100); tft.print("            "); // Ustaw kursor na pozycji (0, 105) i wyświetl pusty tekst o długości 12 znaków, żeby wyczyścić parametry streamu
    tft.setCursor(0, 130); tft.print("                                                                              "); // Analogicznie czyścimy poprzedni tytuł streamu
    tft.setTextColor(TFT_MAGENTA); tft.setCursor(0,STACJE_POS_Y); tft.print(Lista_Stacji[g_Stacja][0]);  // Wyświetl nazwę wybranej stacji na pozycji (0, 65)
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
    tft.setTextSize(2); //Ustaw rozmiar czcionki 2
    tft.setTextColor(TFT_WHITE, TFT_BLACK); // Ustaw kolor czcionki - biały na czarnym tle - do czyszczenia nazwy stacji i streamu
    tft.setCursor(0,STACJE_POS_Y); tft.print("            "); // Ustaw kursor na pozycji (0, STACJE_POS_Y) i wyświetl pusty tekst o długości 12 znaków, żeby wyczyścić poprzednią nazwę stacji
    tft.setCursor(0,80); tft.print("            "); // Ustaw kursor na pozycji (0, 80) i wyświetl pusty tekst o długości 12 znaków, żeby wyczyścić parametry streamu
    tft.setCursor(0,100); tft.print("            "); // Ustaw kursor na pozycji (0, 105) i wyświetl pusty tekst o długości 12 znaków, żeby wyczyścić parametry streamu
    tft.setCursor(0, 130); tft.print("                                                                              "); // Analogicznie czyścimy poprzedni tytuł streamu
    tft.setTextColor(TFT_MAGENTA); tft.setCursor(0,STACJE_POS_Y); tft.print(Lista_Stacji[g_Stacja][0]);  // Wyświetl nazwę wybranej stacji na pozycji (0, 65)
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
    audio.connecttohost(Lista_Stacji[g_Stacja][1]); // Połącz ze stacją o ustawionym indeksie g_Stacja
  }

  // ------------------- S T A C J E ---------------------//

  audio.loop(); // Główna pętla odtwarzacza

  
  // ----------------------- V U -------------------------//
  if ((millis() - g_czasOstatniegoNarysowaniaVU >= 50) && !g_aktywnaZmianaStacji) { // Wskaźnik VU będziemy wyświetlali co 50ms;
    drawVU(VU_POS_X, VU_POS_Y); // Rysujemy wskaźnik VU na pozycji (VU_POS_X, VU_POS_Y) <- współrzędne lewego dolnego rogu wskaźnika
    g_czasOstatniegoNarysowaniaVU = millis(); // Aktualizujemy zmienną przechowującą czas ostatniego narysowania wskaźnika
  }
  // ----------------------- V U -------------------------//

  // --------------------- W i F i -----------------------//
  if (millis() - g_czasOstatniegoStatusuWiFi >= 1000) { // Status WiFi będziemy wyświetlali co 1s;  
    wyswietlStatusWiFi(); // Wyświetlamy status WiFi
    g_czasOstatniegoStatusuWiFi = millis(); // Aktualizujemy zmienną przechowującą czas ostatniego wyświetlenia statusu WiFi
  }  
  // --------------------- W i F i -----------------------//

  vTaskDelay(1); // Opóźnienie 1ms zapobiegające ew. błędom Watchdog Error
}
