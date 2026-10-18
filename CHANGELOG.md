# Changelog

## 0.5.0
- **Transition** on the tile: the old photo slides away to the left (as before) or to the right, or the next one is
  there at once. It takes the place of the Slide toggle; a tile with Slide off gets None.
- **Where the time stands**, on the tile, in the screensaver step and on the Photos face: with the details, or in a
  corner of its own (Time stands).
- **Order of the slideshow** in Home Assistant (`input_select.slideshow_order`): shuffled, by name, or by the date
  taken (from Immich). Take the new homeassistant/photo_slideshow.yaml along. The caption's numbers follow the order.
- Known: the screensaver step of its own shows no photo with Tessera 0.4.89 (the app does not tell the screen the
  step's entity, so it refuses the step's picture); its details do show. Until Tessera fixes it, use Tessera's own
  Photos step with this plugin's caption face.

## 0.4.0
- **Choose what stands over the photo**, on the tile and in the screensaver, each on or off: the album and numbers
  (Holiday · 12/148), the photo's name, the place it was taken (Granada, Spain), the date it was taken, and the time.
  And the corner the details stand in.
- **A screensaver step of its own: Photo slideshow.** The slideshow's own photo over the whole glass in standby, with
  the same details as the tile, and seconds between photos of its own. Add it under Screensaver → Add from plugins.
  Tessera's Photos step and its caption face stay too, for any media folder.
- The name, place and date come from Immich: take the new homeassistant/immich_sync.yaml and photo_slideshow.yaml
  along. Photos from a folder without Immich show their file name.

## 0.3.0
- Needs Tessera 0.4.89 (firmware 0.57.0, plugin API 0.14).
- **A face for Tessera's own Photos screensaver.** Tessera now shows the photos of a media folder in standby by
  itself. Pick **Photo slideshow caption** under Face in the drawer of Photos, and the screensaver shows the same
  caption as the tile over its photo: the album (or words of your own) and which photo of how many, in a corner you
  choose, with the time if you like.
- The Home Assistant package offers the slideshow folder as a media folder too (`media_dirs`), so the Photos step can
  show the same photos without internet. The image entity is no longer needed for the screensaver.

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
