/*
 * oled_utils.h — testo, layout ed elementi grafici per display SSD1306
 * Versione 2
 *
 *   #include "oled_utils.h"
 *   Adafruit_SSD1306 display(128, 64, &Wire, -1);
 *   oled_init(display);
 *
 * Regole generali
 *  - Il testo va a capo da solo (a parole) e non esce mai dallo schermo:
 *    le righe che non entrano in verticale non vengono disegnate.
 *  - Il testo invertito ha un bordo bianco attorno (padding, default 2 px).
 *  - Le coordinate (x, y) indicano sempre l'angolo in alto a sinistra
 *    dell'elemento, bordo compreso.
 *  - Tutto viene disegnato nel buffer: lo schermo cambia solo con oled_aggiorna().
 */
#pragma once
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// ---------- font di default ----------
#define OLED_LARG_CAR 6   // 5 px carattere + 1 px spazio
#define OLED_ALT_CAR  8   // 7 px carattere + 1 px spazio

enum Allinea { SX, CENTRO, DX };

// ---------- stato interno ----------
static Adafruit_SSD1306 *_oled = nullptr;
static int16_t _oled_y          = 0;   // y della prossima riga
static uint8_t _oled_interlinea = 2;   // px extra tra le righe
static uint8_t _oled_margine    = 0;   // margine sx/dx
static uint8_t _oled_pad        = 2;   // bordo attorno al testo invertito


// =====================================================================
//  FUNZIONI INTERNE
// =====================================================================

// Larghezza VISIBILE di n caratteri (senza lo spazio finale)
inline int16_t _oled_vis(int n, uint8_t size) {
  return n > 0 ? n * OLED_LARG_CAR * size - size : 0;
}

// Spostamento per allineare un oggetto largo w in uno spazio largo spazio
inline int16_t _oled_offset(Allinea a, int16_t spazio, int16_t w) {
  if (a == CENTRO) return (spazio - w) / 2;
  if (a == DX)     return spazio - w;
  return 0;
}

// Estrae la prossima riga (max maxc caratteri) a partire da pos.
// Va a capo sugli spazi; spezza le parole solo se più lunghe di una riga.
// Rispetta anche '\n'.
inline String _oled_prossima_riga(const String &t, int &pos, int maxc) {
  int n = t.length();
  while (pos < n && t[pos] == ' ') pos++;          // salta spazi iniziali

  int fine = pos, spazio = -1;
  while (fine < n && t[fine] != '\n' && (fine - pos) < maxc) {
    if (t[fine] == ' ') spazio = fine;
    fine++;
  }

  String riga;
  if (fine >= n || t[fine] == '\n' || t[fine] == ' ') {
    riga = t.substring(pos, fine);                 // testo finito / a capo / spazio
    pos = fine + 1;
  } else if (spazio > pos) {
    riga = t.substring(pos, spazio);               // spezza all'ultimo spazio
    pos = spazio + 1;
  } else {
    riga = t.substring(pos, fine);                 // parola troppo lunga: taglio
    pos = fine;
  }
  riga.trim();
  return riga;
}

// Taglia un testo a maxc caratteri
inline String _oled_taglia(const String &t, int maxc) {
  if (maxc <= 0) return "";
  return (int)t.length() > maxc ? t.substring(0, maxc) : t;
}

// Imposta size e colore del testo (sfondo trasparente)
inline void _oled_stile(uint8_t size, bool nero) {
  _oled->setTextSize(size);
  _oled->setTextColor(nero ? SSD1306_BLACK : SSD1306_WHITE);
}

// Cuore della libreria: blocco di testo con a capo automatico.
//   x0, larg : area orizzontale disponibile
//   y        : bordo superiore del blocco
//   y_max    : limite inferiore (escluso)
//   disegna  : false = misura soltanto
//   completo : (opzionale) false se qualche riga non è entrata
// Ritorna l'altezza del blocco (0 se non entra nulla).
inline int16_t _oled_blocco(int16_t x0, int16_t larg, int16_t y,
                            const String &testo, uint8_t size, Allinea a,
                            bool inv, int16_t y_max, bool disegna = true,
                            bool *completo = nullptr) {
  int16_t pad   = inv ? _oled_pad : 0;
  int16_t passo = OLED_ALT_CAR * size + _oled_interlinea;
  int16_t h_ult = inv ? 7 * size : OLED_ALT_CAR * size;   // altezza ultima riga
  int     maxc  = (larg - 2 * pad + size) / (OLED_LARG_CAR * size);
  if (completo) *completo = false;
  if (maxc < 1) return 0;

  // 1° passaggio: conta righe e larghezza massima
  int n = 0, pos = 0, len = testo.length();
  int16_t wmax = 0;
  while (pos < len) {
    String r = _oled_prossima_riga(testo, pos, maxc);
    int16_t w = _oled_vis(r.length(), size);
    if (w > wmax) wmax = w;
    n++;
  }
  if (n == 0) return 0;

  // quante righe entrano in verticale
  int ok = 0;
  while (ok < n && y + 2 * pad + ok * passo + h_ult <= y_max) ok++;
  if (completo) *completo = (ok == n);
  if (ok == 0) return 0;

  int16_t box_w = wmax + 2 * pad;
  int16_t box_h = 2 * pad + (ok - 1) * passo + h_ult;
  if (!disegna) return box_h;

  int16_t box_x = x0 + _oled_offset(a, larg, box_w);
  if (inv) {
    if (pad >= 2) _oled->fillRoundRect(box_x, y, box_w, box_h, 2, SSD1306_WHITE);
    else          _oled->fillRect(box_x, y, box_w, box_h, SSD1306_WHITE);
  }
  _oled_stile(size, inv);

  // 2° passaggio: disegna
  pos = 0;
  for (int i = 0; i < ok; i++) {
    String r = _oled_prossima_riga(testo, pos, maxc);
    int16_t lx = box_x + pad + _oled_offset(a, wmax, _oled_vis(r.length(), size));
    _oled->setCursor(lx, y + pad + i * passo);
    _oled->print(r);
  }
  return box_h;
}

inline int16_t _oled_W() { return _oled->width(); }
inline int16_t _oled_H() { return _oled->height(); }
inline int16_t _oled_larg_utile() { return _oled_W() - 2 * _oled_margine; }


// =====================================================================
//  BASE
// =====================================================================

inline void oled_pulisci() {
  _oled->clearDisplay();
  _oled_y = 0;
}

inline void oled_aggiorna() {
  _oled->display();
}

inline bool oled_init(Adafruit_SSD1306 &d,
                      uint8_t indirizzo  = 0x3C,
                      uint8_t interlinea = 2,
                      uint8_t margine    = 0,
                      uint8_t pad        = 2) {
  _oled = &d;
  if (!_oled->begin(SSD1306_SWITCHCAPVCC, indirizzo)) return false;
  _oled->setTextWrap(false);   // il wrap lo gestisce la libreria
  _oled->cp437(true);          // "\xF8" = simbolo °
  _oled_interlinea = interlinea;
  _oled_margine    = margine;
  _oled_pad        = pad;
  oled_pulisci();
  oled_aggiorna();
  return true;
}

inline void oled_imposta_interlinea(uint8_t px) { _oled_interlinea = px; }
inline void oled_imposta_margine(uint8_t px)    { _oled_margine = px; }
inline void oled_imposta_pad(uint8_t px)        { _oled_pad = px; }


// =====================================================================
//  MISURE
// =====================================================================

// Larghezza visibile di un testo su una riga
inline uint16_t oled_larghezza(const String &testo, uint8_t size = 1) {
  return _oled_vis(testo.length(), size);
}

// Altezza di una riga di testo (8 * size)
inline uint8_t oled_altezza(uint8_t size = 1) {
  return OLED_ALT_CAR * size;
}

// Caratteri che stanno in una riga (margini esclusi)
inline uint8_t oled_max_caratteri(uint8_t size = 1) {
  return (_oled_larg_utile() + size) / (OLED_LARG_CAR * size);
}

// Altezza che occuperebbe un testo con l'a capo automatico (senza disegnarlo)
inline int16_t oled_misura(const String &testo, uint8_t size = 1, bool inv = false,
                           int16_t larghezza = -1) {
  if (larghezza < 0) larghezza = _oled_larg_utile();
  return _oled_blocco(0, larghezza, 0, testo, size, SX, inv, 32767, false);
}


// =====================================================================
//  CURSORE DI RIGA
// =====================================================================

inline int16_t oled_y()               { return _oled_y; }
inline void    oled_vai_a(int16_t y)  { _oled_y = y; }
inline void    oled_spazio(int16_t px){ _oled_y += px; }

// Spazio verticale rimasto sotto il cursore
inline int16_t oled_spazio_rimasto()  { return _oled_H() - _oled_y; }


// =====================================================================
//  TESTO
// =====================================================================

// Testo con a capo automatico alla y corrente; poi il cursore scende.
// Ritorna false se il testo non è entrato tutto.
inline bool oled_riga(const String &testo, uint8_t size = 1,
                      Allinea a = SX, bool inv = false) {
  bool completo;
  int16_t h = _oled_blocco(_oled_margine, _oled_larg_utile(), _oled_y,
                           testo, size, a, inv, _oled_H(), true, &completo);
  if (h > 0) _oled_y += h + _oled_interlinea;
  return completo;
}

// Testo in posizione libera (x, y), va a capo entro il bordo destro
// dello schermo (o entro 'larghezza' se indicata). Non sposta il cursore.
// Ritorna l'altezza occupata.
inline int16_t oled_testo(int16_t x, int16_t y, const String &testo,
                          uint8_t size = 1, bool inv = false,
                          int16_t larghezza = -1) {
  if (larghezza < 0) larghezza = _oled_W() - _oled_margine - x;
  return _oled_blocco(x, larghezza, y, testo, size, SX, inv, _oled_H());
}

// Testo allineato alla y indicata. Non sposta il cursore.
inline int16_t oled_testo_allineato(int16_t y, const String &testo, uint8_t size = 1,
                                    Allinea a = SX, bool inv = false) {
  return _oled_blocco(_oled_margine, _oled_larg_utile(), y,
                      testo, size, a, inv, _oled_H());
}

// Etichetta a sinistra, valore a destra (una riga sola).
// Se non c'è spazio l'etichetta viene accorciata.
inline bool oled_valore(const String &etichetta, const String &valore, uint8_t size = 1) {
  int16_t h = OLED_ALT_CAR * size;
  if (_oled_y + h > _oled_H()) return false;

  int16_t larg   = _oled_larg_utile();
  int16_t w_val  = _oled_vis(valore.length(), size);
  int     max_et = (larg - w_val - OLED_LARG_CAR * size) / (OLED_LARG_CAR * size);

  _oled_stile(size, false);
  _oled->setCursor(_oled_margine, _oled_y);
  _oled->print(_oled_taglia(etichetta, max_et));
  _oled->setCursor(_oled_margine + larg - w_val, _oled_y);
  _oled->print(valore);

  _oled_y += h + _oled_interlinea;
  return true;
}

// Fascia piena a tutta larghezza con testo centrato in negativo
inline void oled_titolo(const String &testo, uint8_t size = 1) {
  int16_t h = 7 * size + 2 * _oled_pad;
  if (_oled_y + h > _oled_H()) return;
  _oled->fillRect(0, _oled_y, _oled_W(), h, SSD1306_WHITE);

  int maxc = (_oled_W() - 2 * _oled_pad + size) / (OLED_LARG_CAR * size);
  String t = _oled_taglia(testo, maxc);
  _oled_stile(size, true);
  _oled->setCursor((_oled_W() - _oled_vis(t.length(), size)) / 2, _oled_y + _oled_pad);
  _oled->print(t);

  _oled_y += h + _oled_interlinea;
}


// =====================================================================
//  ELEMENTI DI LAYOUT (usano il cursore di riga)
// =====================================================================

inline void oled_separatore(uint8_t spazio = 2) {
  _oled_y += spazio;
  _oled->drawFastHLine(_oled_margine, _oled_y, _oled_larg_utile(), SSD1306_WHITE);
  _oled_y += 1 + spazio;
}

// Separatore tratteggiato
inline void oled_separatore_tratteggiato(uint8_t spazio = 2, uint8_t passo = 3) {
  _oled_y += spazio;
  for (int16_t x = _oled_margine; x < _oled_W() - _oled_margine; x += passo)
    _oled->drawPixel(x, _oled_y, SSD1306_WHITE);
  _oled_y += 1 + spazio;
}


// =====================================================================
//  ELEMENTI GRAFICI (posizione libera)
// =====================================================================

// ---------- barre ----------

// Barra orizzontale con bordo
inline void oled_barra_xy(int16_t x, int16_t y, int16_t w, int16_t h, float perc) {
  perc = constrain(perc, 0, 100);
  _oled->drawRect(x, y, w, h, SSD1306_WHITE);
  int16_t pieno = (int16_t)((w - 4) * perc / 100.0 + 0.5);
  if (pieno > 0) _oled->fillRect(x + 2, y + 2, pieno, h - 4, SSD1306_WHITE);
}

// Barra verticale (si riempie dal basso)
inline void oled_barra_v(int16_t x, int16_t y, int16_t w, int16_t h, float perc) {
  perc = constrain(perc, 0, 100);
  _oled->drawRect(x, y, w, h, SSD1306_WHITE);
  int16_t pieno = (int16_t)((h - 4) * perc / 100.0 + 0.5);
  if (pieno > 0) _oled->fillRect(x + 2, y + h - 2 - pieno, w - 4, pieno, SSD1306_WHITE);
}

// Barra a tutta larghezza alla y corrente (usa il cursore di riga)
inline void oled_barra(float perc, uint8_t altezza = 7) {
  if (_oled_y + altezza > _oled_H()) return;
  oled_barra_xy(_oled_margine, _oled_y, _oled_larg_utile(), altezza, perc);
  _oled_y += altezza + _oled_interlinea;
}

// ---------- cornice ----------

// Riquadro con angoli arrotondati e, se indicato, un'etichetta
// invertita in alto a sinistra (linguetta).
// Ritorna la y da cui iniziare a scrivere il contenuto.
inline int16_t oled_cornice(int16_t x, int16_t y, int16_t w, int16_t h,
                            const String &titolo = "", uint8_t raggio = 3) {
  _oled->drawRoundRect(x, y, w, h, raggio, SSD1306_WHITE);
  if (titolo.length() == 0) return y + 3;

  int maxc = (w - 2 * _oled_pad - 2 * raggio + 1) / OLED_LARG_CAR;
  String t = _oled_taglia(titolo, maxc);
  int16_t tw = _oled_vis(t.length(), 1) + 2 * _oled_pad;
  int16_t th = 7 + 2 * _oled_pad;
  _oled->fillRoundRect(x, y, tw + raggio, th, raggio, SSD1306_WHITE);
  _oled_stile(1, true);
  _oled->setCursor(x + _oled_pad + 1, y + _oled_pad);
  _oled->print(t);
  return y + th + 2;
}

// ---------- icone di stato ----------

// Batteria 14x7 px. inv = true per disegnarla nera (es. dentro oled_titolo)
inline void oled_batteria(int16_t x, int16_t y, float perc, bool inv = false) {
  uint16_t c = inv ? SSD1306_BLACK : SSD1306_WHITE;
  perc = constrain(perc, 0, 100);
  _oled->drawRect(x, y, 12, 7, c);
  _oled->fillRect(x + 12, y + 2, 2, 3, c);
  int16_t pieno = (int16_t)(8 * perc / 100.0 + 0.5);
  if (pieno > 0) _oled->fillRect(x + 2, y + 2, pieno, 3, c);
}

// Tacche di segnale 11x8 px, livello 0-4. inv = true per disegnarle nere
inline void oled_segnale(int16_t x, int16_t y, uint8_t livello, bool inv = false) {
  uint16_t c = inv ? SSD1306_BLACK : SSD1306_WHITE;
  for (uint8_t i = 0; i < 4; i++) {
    int16_t h  = 2 + 2 * i;
    int16_t bx = x + i * 3;
    if (i < livello) _oled->fillRect(bx, y + 8 - h, 2, h, c);
    else             _oled->drawFastHLine(bx, y + 7, 2, c);
  }
}

// Icone 8x8 (1 byte per riga, bit più a sinistra = pixel a sinistra)
static const uint8_t ICONA_TERMOMETRO[8] PROGMEM = {0x18,0x1A,0x18,0x1A,0x3C,0x7E,0x7E,0x3C};
static const uint8_t ICONA_GOCCIA[8]     PROGMEM = {0x18,0x18,0x3C,0x7E,0x7E,0x7E,0x3C,0x00};
static const uint8_t ICONA_OK[8]         PROGMEM = {0x00,0x01,0x03,0x86,0xCC,0x78,0x30,0x00};
static const uint8_t ICONA_ERRORE[8]     PROGMEM = {0xC3,0xE7,0x7E,0x3C,0x3C,0x7E,0xE7,0xC3};
static const uint8_t ICONA_ATTENZIONE[8] PROGMEM = {0x18,0x3C,0x24,0x66,0x7E,0xE7,0xFF,0x00};
static const uint8_t ICONA_PRESSIONE[8]  PROGMEM = {0x3C,0x42,0x99,0xA5,0xA5,0x81,0x42,0x3C};

inline void oled_icona(int16_t x, int16_t y, const uint8_t *icona, bool inv = false) {
  if (inv) {
    _oled->fillRect(x - 1, y - 1, 10, 10, SSD1306_WHITE);
    _oled->drawBitmap(x, y, icona, 8, 8, SSD1306_BLACK);
  } else {
    _oled->drawBitmap(x, y, icona, 8, 8, SSD1306_WHITE);
  }
}

// Icona + testo sulla stessa riga, alla y corrente
inline bool oled_riga_icona(const uint8_t *icona, const String &testo, uint8_t size = 1) {
  int16_t h = OLED_ALT_CAR * size;
  if (_oled_y + max((int16_t)8, h) > _oled_H()) return false;
  int16_t iy = _oled_y + (h - 8) / 2;
  if (iy < _oled_y) iy = _oled_y;
  oled_icona(_oled_margine, iy, icona);
  int16_t tx = _oled_margine + 11;
  int maxc = (_oled_W() - _oled_margine - tx + size) / (OLED_LARG_CAR * size);
  _oled_stile(size, false);
  _oled->setCursor(tx, _oled_y + (h < 8 ? (8 - h) / 2 : 0));
  _oled->print(_oled_taglia(testo, maxc));
  _oled_y += max((int16_t)8, h) + _oled_interlinea;
  return true;
}

// ---------- indicatore a lancetta ----------

// Semicerchio con lancetta. (cx, cy) = centro della base, r = raggio
inline void oled_indicatore(int16_t cx, int16_t cy, int16_t r, float perc) {
  perc = constrain(perc, 0, 100);
  int16_t px = cx - r, py = cy;
  for (int a = 180; a >= 0; a -= 6) {                    // arco
    float rad = a * PI / 180.0;
    int16_t nx = cx + (int16_t)round(r * cos(rad));
    int16_t ny = cy - (int16_t)round(r * sin(rad));
    _oled->drawLine(px, py, nx, ny, SSD1306_WHITE);
    px = nx; py = ny;
  }
  for (int i = 0; i <= 4; i++) {                         // tacche 0-25-50-75-100
    float rad = (180 - i * 45) * PI / 180.0;
    _oled->drawLine(cx + (int16_t)round((r - 3) * cos(rad)), cy - (int16_t)round((r - 3) * sin(rad)),
                    cx + (int16_t)round(r * cos(rad)),       cy - (int16_t)round(r * sin(rad)),
                    SSD1306_WHITE);
  }
  float rad = (180 - perc * 1.8) * PI / 180.0;           // lancetta
  _oled->drawLine(cx, cy, cx + (int16_t)round((r - 5) * cos(rad)),
                  cy - (int16_t)round((r - 5) * sin(rad)), SSD1306_WHITE);
  _oled->fillCircle(cx, cy, 2, SSD1306_WHITE);
}

// ---------- grafico storico ----------

// Grafico a linea dentro un riquadro.
// Se vmin == vmax la scala si calcola da sola sui dati.
inline void oled_grafico(int16_t x, int16_t y, int16_t w, int16_t h,
                         const float *valori, int n,
                         float vmin = 0, float vmax = 0) {
  _oled->drawRect(x, y, w, h, SSD1306_WHITE);
  if (n < 2) return;
  if (vmin == vmax) {
    vmin = vmax = valori[0];
    for (int i = 1; i < n; i++) {
      if (valori[i] < vmin) vmin = valori[i];
      if (valori[i] > vmax) vmax = valori[i];
    }
    if (vmin == vmax) { vmin -= 1; vmax += 1; }
  }
  int16_t ix = x + 2, iy = y + 2, iw = w - 4, ih = h - 4;
  int16_t px = 0, py = 0;
  for (int i = 0; i < n; i++) {
    float v = constrain(valori[i], vmin, vmax);
    int16_t gx = ix + (int32_t)i * (iw - 1) / (n - 1);
    int16_t gy = iy + ih - 1 - (int16_t)((v - vmin) * (ih - 1) / (vmax - vmin) + 0.5);
    if (i > 0) _oled->drawLine(px, py, gx, gy, SSD1306_WHITE);
    px = gx; py = gy;
  }
}

// Aggiunge un valore in fondo allo storico (scorre a sinistra)
inline void oled_storico_aggiungi(float *storico, int n, float valore) {
  for (int i = 0; i < n - 1; i++) storico[i] = storico[i + 1];
  storico[n - 1] = valore;
}

// ---------- menu ----------

// Lista di voci a partire dalla y corrente; la voce selezionata è invertita.
// Se le voci non entrano, scorre e mostra una barra di scorrimento.
inline void oled_menu(const char *voci[], uint8_t n, uint8_t sel, uint8_t size = 1) {
  int16_t passo    = 7 * size + 2 * _oled_pad;
  int16_t visibili = (_oled_H() - _oled_y) / passo;
  if (visibili < 1 || n == 0) return;
  if (sel >= n) sel = n - 1;

  uint8_t primo = (sel >= visibili) ? sel - visibili + 1 : 0;
  bool    scroll = n > visibili;
  int16_t larg   = _oled_larg_utile() - (scroll ? 4 : 0);
  int     maxc   = (larg - 2 * _oled_pad + size) / (OLED_LARG_CAR * size);
  uint8_t mostrate = min((int)visibili, (int)(n - primo));

  for (uint8_t i = 0; i < mostrate; i++) {
    uint8_t idx = primo + i;
    int16_t y   = _oled_y + i * passo;
    bool    s   = (idx == sel);
    if (s) _oled->fillRoundRect(_oled_margine, y, larg, passo, 2, SSD1306_WHITE);
    _oled_stile(size, s);
    _oled->setCursor(_oled_margine + _oled_pad, y + _oled_pad);
    _oled->print(_oled_taglia(voci[idx], maxc));
  }

  if (scroll) {
    int16_t bx = _oled_W() - _oled_margine - 2;
    int16_t th = mostrate * passo;
    for (int16_t yy = _oled_y; yy < _oled_y + th; yy += 2)
      _oled->drawPixel(bx, yy, SSD1306_WHITE);           // binario tratteggiato
    int16_t ch = max((int16_t)3, (int16_t)(th * visibili / n));
    int16_t cy = _oled_y + (int32_t)(th - ch) * primo / (n - visibili);
    _oled->fillRect(bx - 1, cy, 3, ch, SSD1306_WHITE);
  }
  _oled_y += mostrate * passo + _oled_interlinea;
}

// ---------- popup ----------

// Finestra centrata sopra a quello che c'è già sullo schermo
inline void oled_popup(const String &testo, const String &titolo = "") {
  int16_t W = _oled_W(), H = _oled_H();
  int16_t bw = W - 16, bx = 8, in = 4;
  int16_t tb = titolo.length() ? 7 + 2 * _oled_pad : 0;
  int16_t th = _oled_blocco(0, bw - 2 * in, 0, testo, 1, CENTRO, false, 32767, false);
  int16_t bh = tb + 2 * in + th - 1;
  if (bh > H) bh = H;
  int16_t by = (H - bh) / 2;

  _oled->fillRect(bx - 2, by - 2, bw + 4, bh + 4, SSD1306_BLACK);  // pulisce dietro
  _oled->drawRect(bx, by, bw, bh, SSD1306_WHITE);
  if (tb) {
    _oled->fillRect(bx, by, bw, tb, SSD1306_WHITE);
    int maxc = (bw - 2 * _oled_pad + 1) / OLED_LARG_CAR;
    String t = _oled_taglia(titolo, maxc);
    _oled_stile(1, true);
    _oled->setCursor(bx + (bw - _oled_vis(t.length(), 1)) / 2, by + _oled_pad);
    _oled->print(t);
  }
  _oled_blocco(bx + in, bw - 2 * in, by + tb + in, testo, 1, CENTRO, false, by + bh - 1);
}
