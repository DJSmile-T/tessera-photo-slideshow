# Changelog

## 0.2.0
- **Seconds between photos** on the tile: the screen asks for the next photo itself, every so many seconds (at least
  5). 0 keeps the pace of Home Assistant's slideshow interval, as before.
- The Home Assistant package takes an interval of 0 minutes: then Home Assistant changes the photo only when a screen
  or a tap asks. Take the new homeassistant/photo_slideshow.yaml along to use it.

## 0.1.1
- Photos now come through an image entity of Home Assistant (/api/image_proxy/): the Tessera add-on could not fetch
  /local/ through the Supervisor, so the tile stayed empty. Take the new homeassistant/photo_slideshow.yaml along.

## 0.1.0
- First version: your own photos from Home Assistant on a tile, a new one every few minutes, a tap for the next. Set
  up the Home Assistant package of the README first.
