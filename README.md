# M5Dial v2: Home Assistant remote for the M5Stack Dial

ESPHome + LVGL firmware that turns the [M5Stack Dial](https://docs.m5stack.com/en/core/M5Dial) (ESP32-S3, 240×240 round touchscreen, rotary knob, NFC reader) into a handheld Home Assistant remote.

You can control several entities per domain. Swipe left/right to change page, swipe up/down to change entity on the current page, and turn the knob to adjust the value.

## Features

- **Pages:** Clock, Thermostat, RGBW light, HWB light (hue/whiteness/brightness), Music, NFC tag, Settings.
- **Multiple entities per page:** e.g. 8 radiator valves, 2 lights, 2 media players. The name of the selected entity pops up for 1.5 s.
- **Four view styles** shared by all pages: Analog, 14-segment, Modern, Face. The Face views are not implemented yet: each one is a placeholder that only shows the page name.
- **Music:** volume, title, play/pause, progress and cover art of the selected player. Our players are [Music Assistant](https://music-assistant.io/) entities. The music page only uses standard `media_player.*` actions, so it should work with any HA media player, but it has only been tested with Music Assistant. Starting music from NFC tags requires Music Assistant (`music_assistant.play_media`).
- **NFC:** scanned tags are sent to Home Assistant as `tag_scanned` events, and the dial shows the tag name sent back by HA. HA decides what each tag does (see [NFC tags](#nfc-tags)).
- **Power:** "snow" screensaver after a timeout, then the dial powers off on battery. Both timeouts can be set from HA.

## Controls

| Gesture | Action |
|---|---|
| Swipe ← → | Previous / next page |
| Swipe ↑ ↓ | Next / previous entity on the page (valve, light, player) |
| Turn knob | Adjust value (setpoint, brightness/colour, volume) |
| Click knob | Toggle light, valve mode, play/pause, clock seconds |
| Hold knob 3–5 s, release | Next view style (short beep at 3 s = release now) |
| Hold knob ≥ 5 s | Backlight rescue: brightness 50% + long beep |

The RGBW and HWB pages share the same light selection.

## Thermostat page

The thermostat page is designed for **heating only**. It doesn't support cooling or heat/cool modes: the scales and colours assume the room is warmed up towards the setpoint.

Each entry is a **thermostatic radiator valve** (`climate.*`) plus two independent room sensors:

| Value | Source |
|---|---|
| **PV** (process value) | current temperature measured by the valve (`current_temperature` of the climate entity) |
| **SP** (setpoint) | target temperature of the valve, which the knob changes |
| Room temperature | separate sensor (`temp:`), independent of the valve |
| Room humidity | separate sensor (`hum:`), independent of the valve |

The valve measures temperature right next to the radiator, so PV and the room sensor usually differ.

## Repository layout

```
m5Dial_v2.yaml        main config: entity lists + packages
m5_v2/
  hw.yaml             board, display, touch, NFC, power hold, OTA
  my_conf.yaml        node name, Wi-Fi, API
  my_const.yaml       page ids, view styles, timing constants
  nav.yaml            swipe / knob navigation
  entities.h          per-domain entity lists (C++)
  *_entity.yaml       one instance per HA entity (thermo / light / music)
  *_page.yaml         page definition per domain
  *_view_*.yaml       Analog / 14SEG / Moderno / Face views
  *_act.yaml          knob actions per domain
  settings.yaml, tag_page.yaml, screensaver.yaml
fonts/                DSEG14Classic, Material Design Icons webfont
images/               music cover placeholder
```

Comments in the YAML files are in Italian.

## Requirements

- M5Stack Dial
- ESPHome 2026.9 or later (built and tested with 2026.9.0)
- Home Assistant with the ESPHome integration

## Setup

1. **Secrets:** create `secrets.yaml` next to `m5Dial_v2.yaml`:

   ```yaml
   wifi_ssid: "..."
   wifi_password: "..."
   wifi2_ssid: "..."             # second Wi-Fi network
   wifi2_password: "..."
   m5dial_bimbi_api_encryption_key: "..."   # 32-byte base64 key, see ESPHome docs
   ota_password: "..."
   ```

2. **Entities:** edit the list at the top of `m5Dial_v2.yaml`, one line per entity. `idx` sets the swipe order and must be unique within each domain. To add or remove an entity, add or remove its line and recompile:

   ```yaml
   thermo_8: !include {file: m5_v2/thermo_entity.yaml, vars: {idx: 8, name: "Laundry", climate: climate.laundry_valve, temp: sensor.laundry_temperature, hum: sensor.laundry_humidity}}
   light_2:  !include {file: m5_v2/light_entity.yaml,  vars: {idx: 2, name: "Bedroom", light: light.bedroom_led}}
   music_2:  !include {file: m5_v2/music_entity.yaml,  vars: {idx: 2, name: "Bath",    player: media_player.bath}}
   ```

   At boot, each `*_entity.yaml` registers its entity in the list for its domain (`entities.h`). Pages always read the selected entry, so switching entity redraws the page immediately from the last known state.

3. **Flash:** for the first flash, connect over USB and run `esphome run m5Dial_v2.yaml`. Later flashes can go over OTA.

4. **Home Assistant:** add the device from the ESPHome integration. In the device options, enable **"Allow the device to perform Home Assistant actions"**. Without it, the knob and click actions are silently rejected.

## Optional: cover art

The Dial has no PSRAM, so it can't decode JPEG covers. Home Assistant converts the cover to a small BMP that the Dial downloads from `/local/m5dial_cover.bmp`. You need:

- `shell_command` in `configuration.yaml`:
  ```yaml
  shell_command:
    m5dial_cover_bmp: "python3 /config/scripts/m5dial_cover_bmp.py {{ url }}"
  ```
- the conversion script `scripts/m5dial_cover_bmp.py`
- an automation that runs this command when the selected player's `entity_picture` changes and then calls the Dial's `esphome.<node>_cover_ready` action. The Dial also triggers the automation itself when you change player and when HA connects.

Without this setup, the music page shows a placeholder image.

## NFC tags

The dial only reads the tag. It sends a `tag_scanned` event to Home Assistant and opens the TAG page. If HA doesn't call the dial's `esphome.<node>_tag_result` action within a timeout, the page shows "HA non risponde" (HA is not responding).

**What a tag actually does is decided entirely in Home Assistant.** Any automation that triggers on `tag_scanned` can be used.

This is the automation we use. It reads the tag name, starts the music on the player currently selected on the dial (through Music Assistant) and sends the dial what to show. It expects tag names in this format:

```
<Label> # <type> <source> <value>
```

- `type`: `playlist`, `artist`/`artista`, `album`, `brano` (track) or `brani` (tracks)
- `source`: `spotify` or `locale` (local library)
- `value`: a Spotify link/URI, which can be several separated by spaces for tracks, or a local name (`Artist - Album` for albums)

Examples: `Pink Floyd # artist spotify spotify:artist:0k17h0D3J5VfsdmQ1iZtE9`, `Abbey Road # album locale The Beatles - Abbey Road`.

If the name doesn't follow this format, the dial shows N/A and no music starts. The automation finds the dial through its `nome_nodo` sensor, so it doesn't need a hard-coded device id. The titles it sends to the dial are in Italian.

```yaml
- id: m5dial_tag_feedback
  alias: M5Dial - tag scanned
  description: >-
    When a tag is scanned by the dial: parse the tag name, start the music on the
    dial's selected player ("Media player" sensor) and tell the dial what to show
    on the TAG page.
  triggers:
  - trigger: event
    event_type: tag_scanned
  conditions:  # only tags scanned by an M5Dial (device has a "nome_nodo" sensor)
  - condition: template
    value_template: >-
      {{ device_entities(trigger.event.data.device_id | default('', true))
         | select('search', 'nome_nodo') | list | count > 0 }}
  variables:
    tag_id: '{{ trigger.event.data.tag_id }}'
    raw_name: "{{ trigger.event.data.name | default('', true) }}"
    known: "{{ raw_name not in ['', 'None', 'Tag ' ~ tag_id] }}"
    dial_entities: '{{ device_entities(trigger.event.data.device_id) }}'
    node: "{{ states(dial_entities | select('search', 'nome_nodo') | first) | replace('-', '_') }}"
    player: "{{ states(dial_entities | select('search', 'media_player') | first) }}"
    parsed: >-
      {% set types = {'playlist': 'Playlist', 'artist': 'Artista', 'artista': 'Artista',
         'album': 'Album', 'brano': 'Brano', 'brani': 'Brani', 'track': 'Brano'} %}
      {% set sources = {'spotify': 'Spotify', 'locale': 'Locale'} %}
      {% set m = raw_name | regex_findall('^\s*(.+?)\s*#\s*(\S+)\s+(\S+)\s+(\S.*?)\s*$') %}
      {% if known and m and m[0][1] | lower in types and m[0][2] | lower in sources %}
        {% set ma_types = {'playlist': 'playlist', 'artist': 'artist', 'artista': 'artist',
           'album': 'album', 'brano': 'track', 'brani': 'track', 'track': 'track'} %}
        {% set type = ma_types[m[0][1] | lower] %}
        {% set value = m[0][3] %}
        {% set artist = '' %}
        {% if m[0][2] | lower == 'locale' and type == 'album' and ' - ' in value %}
          {% set artist = value.split(' - ', 1)[0] | trim %}
          {% set value = value.split(' - ', 1)[1] | trim %}
        {% endif %}
        {% if type == 'track' %}{% set value = value.replace(',', ' ').split() %}{% endif %}
        {{ {'ok': true, 'label': m[0][0], 'type': type, 'media_id': value, 'artist': artist,
            'title': sources[m[0][2] | lower] ~ ' - ' ~ types[m[0][1] | lower]} }}
      {% else %}{{ {'ok': false} }}{% endif %}
  actions:
  - action: esphome.{{ node }}_tag_result
    data:
      tag_id: '{{ tag_id }}'
      title: "{{ parsed.title if parsed.ok else '' }}"
      content: "{{ parsed.label if parsed.ok else '' }}"
      known: '{{ known }}'
  - condition: template
    value_template: "{{ parsed.ok and player.startswith('media_player.') }}"
  - action: music_assistant.play_media
    target:
      entity_id: '{{ player }}'
    data: >-
      {% if parsed.artist %}
        {{ {'media_type': parsed.type, 'media_id': parsed.media_id, 'artist': parsed.artist, 'enqueue': 'replace'} }}
      {% else %}
        {{ {'media_type': parsed.type, 'media_id': parsed.media_id, 'enqueue': 'replace'} }}
      {% endif %}
  mode: single
```

## Limitations / TODO

- Music has only been tested with Music Assistant players, and NFC tag playback requires Music Assistant.
- Only RGBW lights are supported. Dimmable, RGB, RGBWW and on/off lights are not supported yet.
- Entities are defined at compile time on purpose. Adding or removing entities at runtime (e.g. from HA) is not planned: to change the lists, edit `m5Dial_v2.yaml` and reflash.
- Face views are placeholders and not implemented yet.
- The thermostat page is for heating only (no cooling).
- Flash usage is about 86%, so watch firmware size when adding features.

## License

MIT for the YAML/C++ in this repository. Fonts keep their own licenses: DSEG14 is under SIL OFL 1.1, and Material Design Icons is under the Pictogrammers license.
