# Fotodiavoorstelling

Je eigen foto's (een vakantie, de kinderen, de hond) groot op een Tessera-scherm. Home Assistant kiest om de paar
minuten een foto; de tegel toont hem, schuift hem weg als de volgende komt, en een tik op de tegel vraagt meteen om de
volgende. Het onderschrift onderaan noemt het album en welke foto van hoeveel ("Vakantie · 12/148").

De foto's komen uit een map in Home Assistant, of uit een album in [Immich](https://immich.app) dat Home Assistant naar
die map kopieert, zodat de diavoorstelling ook zonder internet doorloopt. Dezelfde foto kan Tessera's screensaver zijn.

[English](README.md)

## Hoe het werkt

Home Assistant kiest de foto met een template-sensor (`sensor.slideshow_photo`) en biedt hem aan als image-entiteit
(`image.slideshow_photo`). Tessera's app haalt hem daar op en stuurt hem naar het scherm. Via een image-entiteit, omdat
de app als add-on via de Supervisor met Home Assistant praat, en die geeft `/api/...` door maar de `www`-map
(`/local/...`) niet.

## Instellen

1. Maak de map `/config/www/slideshow` (met de Samba- of File editor-add-on) en zet er foto's in: JPG of PNG, elk
   kleiner dan 4 MB. Of vul hem vanuit Immich: zie "Foto's uit Immich" hieronder.
2. Kopieer [`homeassistant/photo_slideshow.yaml`](homeassistant/photo_slideshow.yaml) naar `/config/packages/`. Verander
   daarin `http://homeassistant.local:8123` in het adres van je Home Assistant zoals je browser hem bereikt.
3. Zet in `configuration.yaml`:
   ```yaml
   homeassistant:
     packages: !include_dir_named packages
     allowlist_external_dirs:
       - /config/www/slideshow
   ```
   Controleer de configuratie (Ontwikkelaarshulpmiddelen → YAML) en herstart Home Assistant.
4. Binnen een minuut staat `sensor.slideshow_photo` op iets als `/api/image_proxy/image.slideshow_photo?v=IMG_0412`,
   en toont `image.slideshow_photo` die foto.
5. Voeg de plugin in Tessera toe aan een scherm: Plugins → **Add with a link**, met het adres van deze repository. Het
   scherm bouwt één keer.
6. Zet in Layout de tegel **Diavoorstelling** uit de groep Plugins neer, zo groot als je wilt, en kies
   `sensor.slideshow_photo` als entiteit. De editor toont alleen het onderschrift; de foto zie je op het scherm zelf.
7. In de inspector: **Passen** (vullen of passend), **Onderschrift** en **Schuiven**.
8. Wil je dat een tik de volgende foto geeft, laat het scherm dan Home Assistant-acties uitvoeren: Instellingen →
   Apparaten & diensten → ESPHome → het scherm → Configureren.

Hoe vaak de foto wisselt is `input_number.slideshow_interval` (minuten), het album in het onderschrift
`input_text.slideshow_album`. Beide staan in Home Assistant: aanpassen zonder het scherm te bouwen.

## Foto's uit Immich

[`homeassistant/immich_sync.yaml`](homeassistant/immich_sync.yaml) laat Home Assistant elk kwartier één Immich-album
naar `/config/www/slideshow` kopiëren, als Immich' eigen JPG-voorbeelden (ongeveer 1440 pixels, dus HEIC en de grens van
4 MB doen er niet meer toe). Foto's die er al zijn worden niet opnieuw gehaald: in een vakantiehuis of caravan loopt de
diavoorstelling door als het internet wegvalt.

1. Maak in Immich een API-sleutel met alleen `asset.read` en `asset.view`.
2. Zet `immich_url`, `immich_search_url` en `immich_api_key` in `/config/secrets.yaml` (bovenaan het bestand staat hoe).
   Let op: dat is de `secrets.yaml` van Home Assistant zelf, niet die van de ESPHome-add-on.
3. Voeg de integratie **Downloader** toe met als map `/config/www`.
4. Kopieer `immich_sync.yaml` naar `/config/packages/` en herstart Home Assistant.
5. Vul het ID van het album (het laatste stuk van het adres in Immich) in bij `input_text.immich_album`. **Immich fetch
   now** begint meteen; daarna elk kwartier, 40 foto's per ronde.

Een foto die je uit het album haalt, blijft in de map staan tot je hem daar weghaalt; van een album worden er
maximaal 1000 genomen.

## Als Tessera's screensaver

Tessera's screensaver neemt een image-entiteit als camera. Open in Tessera het scherm → Settings → **Screensaver**, zet
**Show in standby** aan (en Auto standby), kies `image.slideshow_photo` bij **Camera** en sleep Camera bovenaan. In
standby vult de foto het glas, iets donkerder, en wisselt mee met de diavoorstelling. Linksonder staat de naam van de
entiteit: geef `image.slideshow_photo` in Home Assistant de naam van je album. De Waveshare-schermen van 4,3, 5 en 7
inch gaan nooit in standby en hebben dus geen screensaver.

## Goed om te weten

- **Maximaal 4 MB per foto**, JPG of PNG. Telefoonfoto's zijn vaak groter en iPhones bewaren HEIC: maak eerst kleinere
  kopieën, bijvoorbeeld met ImageMagick: `magick mogrify -format jpg -resize "2000x2000>" -quality 85 *.HEIC`. De
  Immich-sync doet dit voor je.
- Een scherm zonder geheugen voor afbeeldingen (de CYD, de Waveshare 3,5 inch, de Hosyond 4 inch) zegt dat in plaats
  van een foto te tonen.
- De bestandslijst van de Folder-sensor kan lang worden: houd hem uit de geschiedenis met
  `recorder: exclude: entities: [sensor.slideshow]` in `configuration.yaml`.
- Blijft de tegel leeg met alleen het onderschrift: open `image.slideshow_photo` in Home Assistant. Staat daar geen
  foto, dan is het adres in de package (stap 2) niet het adres waarop Home Assistant zichzelf bereikt.

## Privacy

Alles blijft thuis. Tessera's app haalt elke foto bij Home Assistant zelf op en stuurt hem naar het scherm; het scherm
komt nooit op internet. Let op: Home Assistant serveert de `www`-map (`/local/`) zonder inlog aan iedereen die je Home
Assistant kan bereiken en een bestandsnaam kent. De Immich-sync praat alleen met je eigen Immich, met een sleutel die je
bibliotheek kan lezen maar niets kan wijzigen.
