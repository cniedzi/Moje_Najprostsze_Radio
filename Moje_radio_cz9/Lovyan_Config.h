#include <LovyanGFX.hpp>


//***********************************************************************************//
//***********************************************************************************//
//***********************************************************************************//
//
//Poniżej pozostawiamy odkomentowaną tylko jedną linię zawierającą właściwy wyświetlacz
//
//#define ILI9341
#define ST7789


// Tutaj definiujemy piny, których użyliśmy do podłączenia naszego wyświetlacza SPI
#define TFT_DC 14
#define TFT_RST 18
#define TFT_CS 10
#define TFT_BL 21
#define TFT_MOSI 11 // <- WAŻNE: sprzętowe SPI
#define TFT_SCLK 12 // <- WAŻNE: sprzętowe SPI
#define TFT_MISO -1 // Nie używamy MISO
//***********************************************************************************//
//***********************************************************************************//
//***********************************************************************************//



class LGFX : public lgfx::LGFX_Device
{
  #ifdef ILI9341
    lgfx::Panel_ILI9341 _panel_instance;
  #endif
  #ifdef ST7789
    lgfx::Panel_ST7789 _panel_instance;
  #endif
  
  lgfx::Bus_SPI _bus_instance;

public:
  LGFX(void)
  {
    { 
      auto cfg = _bus_instance.config();
      #ifdef ILI9341
        cfg.spi_host = SPI2_HOST;     // ESP32-S2, C3, S3: SPI2_HOST or SPI3_HOST / ESP32 : VSPI_HOST or HSPI_HOST
        cfg.spi_mode = 0;
        cfg.freq_write = 40000000;
        cfg.freq_read  = 16000000;
        cfg.spi_3wire  = false;
        cfg.use_lock   = true;
        #ifdef ESP32P4
          cfg.dma_channel = 0;
        #else
          cfg.dma_channel = SPI_DMA_CH_AUTO; //Ustaw kanał DMA, który ma być używany (0 = brak DMA / 1 = 1 kanał / 2 = kanał / SPI_DMA_CH_AUTO = ustawienie automatyczne)
        #endif
        cfg.pin_sclk = TFT_SCLK;
        cfg.pin_mosi = TFT_MOSI;
        cfg.pin_miso = TFT_MISO;
        cfg.pin_dc   = TFT_DC;
      #endif
      #ifdef ST7789
        cfg.spi_host = SPI2_HOST;     // ESP32-S2, C3, S3: SPI2_HOST or SPI3_HOST / ESP32 : VSPI_HOST or HSPI_HOST
        cfg.spi_mode = 0;
        cfg.freq_write = 80000000;
        cfg.freq_read  = 16000000;
        cfg.spi_3wire  = false;
        cfg.use_lock   = true;
        #ifdef ESP32P4
          cfg.dma_channel = 0;
        #else
          cfg.dma_channel = SPI_DMA_CH_AUTO; //Ustaw kanał DMA, który ma być używany (0 = brak DMA / 1 = 1 kanał / 2 = kanał / SPI_DMA_CH_AUTO = ustawienie automatyczne)
        #endif
        cfg.pin_sclk = TFT_SCLK;
        cfg.pin_mosi = TFT_MOSI;
        cfg.pin_miso = TFT_MISO;
        cfg.pin_dc   = TFT_DC;
      #endif
      _bus_instance.config(cfg);
      _panel_instance.setBus(&_bus_instance);
    }
    { 
      auto cfg = _panel_instance.config();
      #ifdef ILI9341
        cfg.pin_cs        = TFT_CS;
        cfg.pin_rst       = TFT_RST;
        cfg.memory_width  = 240;
        cfg.memory_height = 320;
        cfg.panel_width   = 240;
        cfg.panel_height  = 320;
        cfg.offset_x      = 0;
        cfg.offset_y      = 0;
        cfg.offset_rotation = 0;
        cfg.dummy_read_pixel = 8;
        cfg.dummy_read_bits  = 1;
        cfg.readable     = false;
        cfg.invert       = false;
        cfg.rgb_order    = false;
        cfg.bus_shared   = false;
      #endif
      #ifdef ST7789
        cfg.pin_cs           = TFT_CS;
        cfg.pin_rst          = TFT_RST;
        cfg.memory_width  = 240;  // fizyczna szerokość kontrolera
        cfg.memory_height = 320;  // fizyczna wysokość
        cfg.panel_width   = 240;  // widoczna szerokość
        cfg.panel_height  = 320;  // widoczna wysokość
        cfg.offset_x      = 0;
        cfg.offset_y      = 0;
        cfg.offset_rotation = 0;
        cfg.dummy_read_pixel = 8;
        cfg.dummy_read_bits  = 1;
        cfg.readable     = false; // brak MISO
        cfg.invert       = true;  // UWAGA: większość ST7789V wymaga inwersji kolorów
        cfg.rgb_order    = false; // czasami trzeba dać true jeśli kolory są zamienione
        cfg.bus_shared   = false;
      #endif
      _panel_instance.config(cfg);
    }
    setPanel(&_panel_instance);
  }
};
