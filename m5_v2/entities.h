// Registri delle entita' HA per dominio (valvole, luci RGBW, player musicali).
// Ogni <dominio>_entity.yaml incluso in m5Dial_v2.yaml aggiunge una voce al boot (on_boot),
// quindi la dimensione di ogni lista = numero di righe di include, senza contatori scritti a mano.
// Le voci tengono l'ultimo stato ricevuto da HA: cambiando entita' (swipe su/giu') la pagina si
// ridisegna subito da qui, senza aspettare HA.
#pragma once
#include <algorithm>
#include <cmath>
#include <string>
#include <vector>
#include <cstdio>

struct ThermoEntry {
  int idx;              // ordine di visualizzazione (vars idx del package)
  std::string name;     // nome mostrato sul display
  std::string climate;  // entity_id climate.*
  float t_true = NAN;   // sensore di temperatura della stanza
  float pv = NAN;       // current_temperature della valvola
  float sp = NAN;       // setpoint (temperature)
  float hum = NAN;      // umidita' relativa
  std::string mode;     // off/auto/heat
};

struct LightEntry {
  int idx;
  std::string name;
  std::string entity;   // entity_id light.* (RGBW)
  bool on = false;
  int can_r = 0, can_g = 0, can_b = 0, can_w = 0;  // rgbw_color HA (ultimo valido)
  int bright = 0;                                  // brightness HA 0-255
  int rgb_r = 0, rgb_g = 0, rgb_b = 0;             // rgb_color HA (pallino centrale)
  int hue = 0;                                     // da hs_color
};

struct MusicEntry {
  int idx;
  std::string name;
  std::string entity;   // entity_id media_player.*
  float duration = NAN;
  float pos = NAN;
  std::string pos_upd;  // media_position_updated_at
  std::string track;    // media_content_id
  std::string title;    // media_title
  std::string state;    // playing/paused/...
  float volume = NAN;   // volume_level 0-1
};

// Le tre liste (C++17 inline: una sola istanza). Non sono "globals" ESPHome perche' il codice generato
// dichiara i globals prima di includere questo header, quando questi tipi non esistono ancora.
inline std::vector<ThermoEntry> thermo_list;
inline std::vector<LightEntry> light_list;
inline std::vector<MusicEntry> music_list;

// Inserisce mantenendo l'ordine per idx (l'ordine di esecuzione degli on_boot non e' garantito).
template<typename T> void entity_register(std::vector<T> &list, const T &e) {
  auto it = std::find_if(list.begin(), list.end(), [&](const T &x) { return x.idx > e.idx; });
  list.insert(it, e);
}

// Voce con quel idx (nullptr se non registrata).
template<typename T> T *entity_find(std::vector<T> &list, int idx) {
  for (auto &e : list)
    if (e.idx == idx) return &e;
  return nullptr;
}

// Voce selezionata (nullptr se lista vuota o selezione fuori range).
template<typename T> T *entity_sel(std::vector<T> &list, int sel) {
  return (sel >= 0 && sel < (int) list.size()) ? &list[sel] : nullptr;
}

// true se la voce con quel idx e' quella selezionata.
template<typename T> bool entity_is_sel(std::vector<T> &list, int sel, int idx) {
  T *e = entity_sel(list, sel);
  return e != nullptr && e->idx == idx;
}

// Selezione successiva/precedente con giro (0 se lista vuota).
inline int entity_step(int sel, int size, int dir) {
  if (size <= 0) return 0;
  return ((sel + dir) % size + size) % size;
}

// Tutti gli entity_id separati da virgola (per homeassistant.update_entity).
template<typename T, typename F> std::string entity_join(std::vector<T> &list, F field) {
  std::string out;
  for (auto &e : list) {
    if (!out.empty()) out += ",";
    out += field(e);
  }
  return out;
}

inline float clampf(float v, float lo, float hi) {
  if (std::isnan(v)) return lo;
  return v < lo ? lo : (v > hi ? hi : v);
}

// Valore formattato, "--" se non ancora ricevuto da HA.
inline std::string thermo_fmt(float v, const char *fmt) {
  if (std::isnan(v)) return "--";
  char buf[16];
  snprintf(buf, sizeof(buf), fmt, v);
  return buf;
}

// Player selezionato; voce vuota (entity "") se la lista e' vuota, cosi' i lambda non devono controllare nullptr.
inline MusicEntry &music_cur(std::vector<MusicEntry> &list, int sel) {
  static MusicEntry empty{};
  MusicEntry *e = entity_sel(list, sel);
  return e ? *e : empty;
}

// Luce selezionata; voce vuota se la lista e' vuota.
inline LightEntry &light_cur(std::vector<LightEntry> &list, int sel) {
  static LightEntry empty{};
  LightEntry *e = entity_sel(list, sel);
  return e ? *e : empty;
}
