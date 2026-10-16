# Photo slideshow

Your own photos (a holiday, the kids, the dog) large on a Tessera screen. Home Assistant picks a photo every few
minutes; the tile shows it, slides it away when the next one comes, and a tap on the tile asks for the next photo at
once. The caption at the bottom says the album and which photo of how many ("Holiday · 12/148").

The photos come from a folder in Home Assistant, or from an album in [Immich](https://immich.app) that Home Assistant
copies into that folder, so the slideshow keeps going without internet. The same photo can be Tessera's screensaver.

[Nederlands](README.nl.md)

## How it works

```
/config/www/slideshow/*.jpg ──► sensor.slideshow (Folder: the list of files)
                                  │
                                  ▼
             sensor.slideshow_photo (a template sensor: picks one every few minutes)
                                  │  state: /api/image_proxy/image.slideshow_photo?v=<photo>
                                  ▼
             image.slideshow_photo (an image entity: Home Assistant serves the photo)
                                  │
                                  ▼
             Tessera's app fetches it and sends it to the screen ──► the Slideshow tile
```

The photo goes through an image entity because Tessera's app runs as an add-on: it reaches Home Assistant through the
Supervisor, which passes `/api/...` on but not the `www` folder (`/local/...`).

## Set up

1. Make the folder `/config/www/slideshow` (with the Samba or File editor add-on) and put photos in it: JPG or PNG,
   each under 4 MB. Or fill it from Immich: see "Photos from Immich" below.
2. Copy [`homeassistant/photo_slideshow.yaml`](homeassistant/photo_slideshow.yaml) to `/config/packages/`. In it,
   change `http://homeassistant.local:8123` to the address of your Home Assistant as your browser reaches it.
3. Add to `configuration.yaml`:
   ```yaml
   homeassistant:
     packages: !include_dir_named packages
     allowlist_external_dirs:
       - /config/www/slideshow
   ```
   Check the configuration (Developer tools → YAML) and restart Home Assistant.
4. Within a minute `sensor.slideshow_photo` has a state such as `/api/image_proxy/image.slideshow_photo?v=IMG_0412`,
   and `image.slideshow_photo` shows that photo.
5. Add this plugin to a screen in Tessera: Plugins → **Add with a link**, with this repository's address. The screen
   builds once.
6. In Layout, place the **Slideshow** tile from the library's Plugins group, as large as you like, and choose
   `sensor.slideshow_photo` as its entity. The editor shows the caption only; the photo shows on the screen itself.
7. In the inspector: **Fit** (cover fills the tile, contain shows the whole photo), **Caption** and **Slide**.
8. For a tap to give the next photo, allow the screen to perform Home Assistant actions: Settings → Devices &
   services → ESPHome → the screen → Configure.

How often the photo changes is `input_number.slideshow_interval` (minutes), and the album in the caption is
`input_text.slideshow_album`. Both are in Home Assistant: change them without building the screen.

## Photos from Immich

[`homeassistant/immich_sync.yaml`](homeassistant/immich_sync.yaml) lets Home Assistant copy one Immich album into
`/config/www/slideshow` every 15 minutes, as Immich's own JPG previews (about 1440 pixels, so HEIC and the 4 MB limit
no longer matter). Photos already there are not fetched again: in a holiday home or a caravan, the slideshow keeps
going when the internet is away.

1. In Immich, make an API key with only `asset.read` and `asset.view`.
2. Put `immich_url`, `immich_search_url` and `immich_api_key` in `/config/secrets.yaml` (the comments at the top of the
   file say how). Mind: that is Home Assistant's own `secrets.yaml`, not the ESPHome add-on's.
3. Add the **Downloader** integration with the folder `/config/www`.
4. Copy `immich_sync.yaml` to `/config/packages/` and restart Home Assistant.
5. Fill in the album's id (the last part of its address in Immich) in `input_text.immich_album`. **Immich fetch now**
   starts at once; after that it runs every 15 minutes, 40 photos a round.

A photo you take out of the album stays in the folder until you delete it there; at most 1000 photos of an album are
taken.

## As Tessera's screensaver

Tessera's screensaver takes an image entity as a camera. In Tessera, open the screen → Settings → **Screensaver**,
turn on **Show in standby** (and Auto standby), choose `image.slideshow_photo` under **Camera** and drag Camera to the
top. In standby the photo fills the glass, a little darker, and changes with the slideshow. The name at its bottom
left is the entity's name: rename `image.slideshow_photo` in Home Assistant to show your album instead. The Waveshare
4.3, 5 and 7-inch boards never go into standby, so they have no screensaver.

## Good to know

- **4 MB at most per photo**, JPG or PNG. Photos of a phone are often larger, and iPhones store HEIC: make smaller
  copies first, for instance with ImageMagick: `magick mogrify -format jpg -resize "2000x2000>" -quality 85 *.HEIC`.
  The Immich sync does this for you.
- A board without the memory for pictures (the CYD, the Waveshare 3.5-inch, the Hosyond 4-inch) says so instead.
- The Folder sensor's list of files can be long: keep it out of the history with
  `recorder: exclude: entities: [sensor.slideshow]` in `configuration.yaml`.
- The tile stays empty with only its caption: open `image.slideshow_photo` in Home Assistant. Without a photo there,
  the address in the package (step 2) is not the one Home Assistant reaches itself on.

## Privacy

Everything stays in your home. Tessera's app fetches each photo from Home Assistant itself and sends it to the screen;
the screen never reaches the internet. Mind that Home Assistant serves the `www` folder (`/local/`) without a login to
anyone who can reach your Home Assistant and knows a file's name. The Immich sync talks only to your own Immich, with a
key that can read your library but change nothing.
