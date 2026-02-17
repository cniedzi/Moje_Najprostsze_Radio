// W Arduino IDE musimy zainstalować biblioteki:
// 1. ESP32-audioI2S
// 2. Adafruit GFX Library
// 3. Adafruit ST7735 and ST7789 - bibliotekę dobieramy do naszego wyświetlacza (w tym przykładzie używam wyświetlacza ST7789); np. dla ILI9341 instalujemy bibliotekę Adafruit ILI9341, itd)
#include <WiFi.h>
#include "Audio.h" // (https://github.com/schreibfaul1/ESP32-audioI2S)
#include "Adafruit_GFX.h"
#include <Adafruit_ST7789.h> // dołączamy bibliotekę obsługującą nasz wyświetlacz (np. dla ILI9341 będzie to #include <Adafruit_ILI9341.h>, itd)

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

// Definicja niezbędnych obiektów
SPIClass spi(HSPI); // Interfejs SPI
Adafruit_ST7789 tft = Adafruit_ST7789(&spi, TFT_CS, TFT_DC, TFT_RST); // Wyświetlacz SPI
Audio audio; // Odtwarzacz audio

// ------------------ E N K O D E R --------------------//
Rotary EncL = Rotary(S1_EncoderL, S2_EncoderL); // Lewy enkoder do regulacji głośności radia
Rotary EncR = Rotary(S1_EncoderR, S2_EncoderR); // Prawy enkoder do wyboru stacji
// ------------------ E N K O D E R --------------------//


// ------------------- S T A C J E ---------------------//
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
/*Wiersz 0*/    {"Italo4you",   "http://s0.radiohost.pl:8018/stream"},
/*Wiersz 1*/    {"ESKA",        "https://waw.ic.smcdn.pl/2380-1.aac"},
/*Wiersz 2*/    {"RMF FM",      "https://rs101-krk-cyfronet.rmfstream.pl/RMFFM48"},
/*Wiersz 3*/    {"Vox FM",      "https://waw.ic.smcdn.pl/3990-1.aac"},
/*Wiersz 4*/    {"ESKA Rock",   "https://waw.ic.smcdn.pl/5980-1.aac"}   // W ostatnim wierszu nie dajemy na końcu przecinka ','
};

int g_Stacja = 0; // zmienna globalna przechowująca indeks aktualnej stacji, czyli indeks wiersza w Lista_Stacji[g_Stacja][1]; w tym przypadku indeks 0, czyli stację 1 -> "Italo 4 you" :)
// ------------------- S T A C J E ---------------------//


// ----------------- G Ł O Ś N O Ś Ć -------------------//
int g_Volume = 3; // zmienna globalna przechowująca aktualną głośność radia
// ----------------- G Ł O Ś N O Ś Ć -------------------//

// ----------------------- V U -------------------------//
unsigned long g_czasOstatniegoNarysowaniaVU = millis(); // Zmienna, w której będziemy przechowywali czas ostatniego narysowania wskaźnika VU. Funkcja millis() zwraca liczbę milisekund, które upłynęły od momentu włączenia radia do chwili wywołania tej funkcji
// ----------------------- V U -------------------------//


// Opcjonalna funkcja wyświetlająca komunikaty odtwarzacza audio w Serial Monitorze oraz nazwę streamu na wyświetlaczu
void my_audio_info(Audio::msg_t m) {
  Serial.printf("%s: %s\n", m.s, m.msg);
  if (m.e == Audio::evt_streamtitle) {
    tft.setTextSize(2);
    tft.setTextColor(ST77XX_CYAN, ST77XX_BLACK);
    tft.setCursor(0, 140);
    tft.print("                                                                                                        "); // Czyścimy poprzedni tytuł streamu
    tft.setCursor(0, 140);
    tft.print(m.msg);
  }
}



// --------------------- L O G O -----------------------//
// Ta funkcja będzie wyświetlała logo stacji o nazwie nazwaStacji przekazanej jako parametr w wywołaniu funkcji
void wyswietlLogoStacji(String nazwaStacji) { // do funkcji przekazujemy nazwę stacji, dla której chcemy wyświetlić logo
  // Przyjmujemy następujące współrzędne środka logo:
  // x_sr = 260
  // y_sr = 50
  tft.fillRect(200, 50, 120, 60, ST77XX_BLACK); // Czyścimy poprzednie logo przed wyświetleniem nowego
  if (nazwaStacji == "Italo4you") tft.drawRGBBitmap(230, 50, ITALO4YOU, 60, 60);
  else if (nazwaStacji == "ESKA") tft.drawRGBBitmap(200, 50, ESKA, 120, 40);
  else if (nazwaStacji == "RMF FM") tft.drawRGBBitmap(213, 50, RMFFM, 95, 60);
  else if (nazwaStacji == "Vox FM") tft.drawRGBBitmap(230, 50, VOXFM, 60, 60);
  else if (nazwaStacji == "ESKA Rock") tft.drawRGBBitmap(225, 50, ESKAROCK, 70, 60);
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
  tft.setTextColor(ST77XX_WHITE); // Ustawiamy kolor czcionki na biały
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
  
  tft.init(240, 320); // Inicjalizacja wyświetlacza ST7789 ( dla ILI9341 zamiast tego będzie tft.begin(); )
  //tft.invertDisplay(false); // Jeżeli kolory są wyświetlane w negatywie, trzeba odkomentować tę albo następną komendę (tj. usunąć "//" na początku linii)
  //tft.invertDisplay(true); // Jeżeli kolory są wyświetlane w negatywie, trzeba odkomentować tę albo poprzednią komendę (tj. usunąć "//" na początku linii)
  tft.setRotation(1); // Dopasowanie obrotu obrazu ( dla ILI9341 zamiast tego będzie tft.setRotation(3); )
  tft.setTextWrap(true); // Tekst dłuższy niż szerokość ekranu będzie zawijany do kolejnej linii
  tft.fillScreen(ST77XX_BLACK); // Czyścimy ekran - kolor czarny

  // --------------------- L O G O -----------------------//
  // Do wyświetlania bitmap będziemy używać funkcji z biblioteki Adafruit GFX:
  //drawRGBBitmap(x_lewy_górny_wierzchołek_logo, y__lewy_górny_wierzchołek_logo, *bitmapa, szerokosc_bitmapy, wysokosc_bitmapy);
  tft.setTextSize(3); // Ustawiamy rozmiar czcionki wyświetlacza na 3
  tft.setTextColor(ST77XX_ORANGE); // Ustawiamy kolor czcionki wyświetlacza
  tft.setCursor(61,0); // Ustawiamy pozycję (x, y) tekstu, na której będzie wyswietlony tekst
  tft.print("NAJPROSTSZE"); // Wyswietlamy napis "RADIO INTERNETOWE" na pozycji (0, 0)
  tft.setCursor(7,30); // Ustawiamy pozycję (x, y) tekstu, na której będzie wyswietlony tekst
  tft.print("RADIO INTERNETOWE"); // Wyswietlamy napis "RADIO INTERNETOWE" na pozycji (0, 0)
  tft.drawRGBBitmap(20, 77, LOGO, 280, 141); // Wyświetlamy logo startowe; dobieramy współrzedne lewego górnego wierzchołka tak, żeby logo było wycentrowane
  digitalWrite(TFT_BL, HIGH); // Włączamy podświetlenie wyświetlacza
  delay(5000); // Przez 5 sekund podziwiamy logo :)
  tft.fillScreen(ST77XX_BLACK); // Czyścimy ekran - kolor czarny
  // --------------------- L O G O -----------------------//

  tft.setTextSize(3); // Ustawiamy rozmiar czcionki wyświetlacza na 3
  tft.setTextColor(ST77XX_ORANGE); // Ustawiamy kolor czcionki wyświetlacza
  tft.setCursor(0,0); // Ustawiamy pozycję (x, y) tekstu, na której będzie wyswietlony tekst
  tft.print("RADIO INTERNETOWE"); // Wyswietlamy napis "RADIO INTERNETOWE" na pozycji (0, 0)
  tft.setTextSize(2); // Ustawiamy rozmiar czcionki wyświetlacza na 2
  tft.setTextColor(ST77XX_WHITE); // Zmieniamy kolor czcionki na biały
  tft.setCursor(0,50); tft.println("Stacja:"); // Wyświetlamy napis "Stacja:" na pozycji (0, 50)
  tft.setTextColor(ST77XX_MAGENTA); tft.setCursor(0,70); tft.print(Lista_Stacji[g_Stacja][0]); // Zmieniamy kolor czcionki na cyjanowy i wyświetlamy zdefiniowaną wcześniej nazwę stacji na pozycji (0, 70)
  
  // --------------------- L O G O -----------------------//
  wyswietlLogoStacji(Lista_Stacji[g_Stacja][0]);
  // --------------------- L O G O -----------------------//
  
  tft.setTextSize(2); // Ustawiamy rozmiar czcionki wyświetlacza na 2
  tft.setTextColor(ST77XX_WHITE); tft.setCursor(0, 120); tft.print("Stream:"); // Zmieniamy kolor czcionki na biały i wyświetlamy napis "Stream:" na pozycji (0, 120)

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

  // --------- E N K O D E R + G Ł O Ś N O Ś Ć -----------//
  unsigned char result_L = EncL.process(); // sprawdzamy stan lewego enkodera
  if (result_L == DIR_CW && g_Volume < VOLUME_MAX) { // Jeżeli obrót zgodnie z ruchem wskazówek zegara i aktualna głośność jest mniejsza niż wartość maksymalna, to
    g_Volume++; // zwiększ Volume o 1
    audio.setVolume(g_Volume); // Ustawiamy wartość g_Volume jako aktualną głośność radia
    tft.setTextSize(2); // Ustawiamy rozmiar czcionki wyświetlacza na 2
    tft.setTextColor(VOLUME_KOLOR, ST77XX_BLACK); tft.setCursor(48, 225); tft.printf("%02d", g_Volume); // Zmieniamy kolor czcionki na VOLUME_KOLOR i wyświetlamy aktualną wartość głośności na pozycji (96, 225);
                                                                                                        // WAŻNE: ustawiliśmy też czarny kolor tła czcionki, dzięki czemu poprzednia wartość będzie wymazywana,
                                                                                                        // a format wyświetlania "%02d" powoduje, że zawsze wyświetlana jest liczba 2-cyfrowa, np. 01,
                                                                                                        // dzięki czemu przy przejściu z liczby 10 na 9 nie pozostanie stara nadmiarowa cyfra 0.
    Serial.printf("Volume: %d\n", g_Volume); // wyświetlamy aktualną głośność na Serial Monitorze
  }
  else if (result_L == DIR_CCW && g_Volume > VOLUME_MIN) { // Jeżeli obrót przeciwnie do ruchu wskazówek zegara i aktualna głośność jest większa niż wartość minimalna, to
    if (g_Volume > VOLUME_MIN) g_Volume--; // zmniejsz Volume o 1
    audio.setVolume(g_Volume); // Ustawiamy wartość g_Volume jako aktualną głośność radia
    tft.setTextSize(2); // Ustawiamy rozmiar czcionki wyświetlacza na 2
    tft.setTextColor(VOLUME_KOLOR, ST77XX_BLACK); tft.setCursor(48, 225); tft.printf("%02d", g_Volume); // Zmieniamy kolor czcionki na VOLUME_KOLOR i wyświetlamy aktualną wartość głośności na pozycji (96, 225);
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
    tft.setTextColor(ST77XX_WHITE, ST77XX_BLACK); // Ustaw kolor czcionki - biały na czarnym tle - do czyszczenia nazwy stacji i streamu
    tft.setCursor(0,70); tft.print("         "); // Ustaw kursor na pozycji (0, 70) i wyświetl pusty tekst o długości 9 znaków (tyle znaków ma najdłuższa nazwa naszych stacji), żeby wyczyścić poprzednią nazwę stacji
    tft.setCursor(0, 140); tft.print("                                                                                                        "); // Analogicznie czyścimy poprzedni tytuł streamu
    tft.setTextColor(ST77XX_MAGENTA); tft.setCursor(0,70); tft.print(Lista_Stacji[g_Stacja][0]);  // Wyświetl nazwę wybranej stacji na pozycji (0, 70)
    wyswietlLogoStacji(Lista_Stacji[g_Stacja][0]); // Wyświetl logo wybranej stacji
    Serial.printf("Wybrana stacja: %s\n", Lista_Stacji[g_Stacja][0]); // Wyświetl nazwę wybranej stacji w Serial Monitorze
    tft.fillRect(VU_POS_X - 10, VU_POS_Y - 18, 320 - (VU_POS_X - 10), 18, ST77XX_BLACK); // Czyścimy wskaźnik VU
    audio.connecttohost(Lista_Stacji[g_Stacja][1]); // Połącz ze stacją o ustawionym indeksie g_Stacja
  }
  else if (result_R == DIR_CCW && g_Stacja > 0) { // Jeżeli obrót przeciwnie do ruchu wskazówek zegara i aktualny indeks stacji jest większy od indeksu 0, czyli aktualna stacja nie jest pierwszą stacją na liście, to
    g_Stacja--; // zmniejsz indeks stacji g_Stacja o 1
    tft.setTextSize(2); //Ustaw rozmiar czcionki 2
    tft.setTextColor(ST77XX_WHITE, ST77XX_BLACK); // Ustaw kolor czcionki - biały na czarnym tle - do czyszczenia nazwy stacji i streamu
    tft.setCursor(0,70); tft.print("         "); // Ustaw kursor na pozycji (0, 70) i wyświetl pusty tekst o długości 9 znaków (tyle znaków ma najdłuższa nazwa naszych stacji), żeby wyczyścić poprzednią nazwę stacji
    tft.setCursor(0, 140); tft.print("                                                                                                        "); // Analogicznie czyścimy poprzedni tytuł streamu
    tft.setTextColor(ST77XX_MAGENTA); tft.setCursor(0,70); tft.print(Lista_Stacji[g_Stacja][0]);  // Wyświetl nazwę wybranej stacji na pozycji (0, 70)
    wyswietlLogoStacji(Lista_Stacji[g_Stacja][0]); // Wyświetl logo wybranej stacji
    Serial.printf("Wybrana stacja: %s\n", Lista_Stacji[g_Stacja][0]); // Wyświetl nazwę wybranej stacji w Serial Monitorze
    tft.fillRect(VU_POS_X - 10, VU_POS_Y - 18, 320 - (VU_POS_X - 10), 18, ST77XX_BLACK); // Czyścimy wskaźnik VU
    audio.connecttohost(Lista_Stacji[g_Stacja][1]); // Połącz ze stacją o ustawionym indeksie g_Stacja
  }
  // ------------------- S T A C J E ---------------------//

  audio.loop(); // Główna pętla odtwarzacza

  
  // ----------------------- V U -------------------------//
  if (millis() - g_czasOstatniegoNarysowaniaVU >= 50) { // Wskaźnik VU będziemy wyświetlali co 50ms;
    drawVU(VU_POS_X, VU_POS_Y); // Rysujemy wskaźnik VU na pozycji (VU_POS_X, VU_POS_Y) <- współrzędne lewego dolnego rogu wskaźnika
    g_czasOstatniegoNarysowaniaVU = millis(); // Aktualizujemy zmienną przechowującą czas ostatniego narysowania wskaźnika
  }
  // ----------------------- V U -------------------------//

  vTaskDelay(1); // Opóźnienie 1ms zapobiegające ew. błędom Watchdog Error
}
