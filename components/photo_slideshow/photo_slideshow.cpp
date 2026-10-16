#include "photo_slideshow.h"
#include <algorithm>
#include <string>

namespace esphome::photo_slideshow {

using tessera::Font;
namespace ui = tessera::ui;

// Home Assistant's own pictures the app may fetch for a tile of an entity: an image entity (/api/image_proxy/), an
// uploaded image (/api/image/serve/) or the www folder (/local/, which the Tessera add-on cannot reach through the
// Supervisor: the README's package uses an image entity).
static bool is_photo(const std::string &address) {
  return address.rfind("/api/image_proxy/", 0) == 0 || address.rfind("/api/image/serve/", 0) == 0 ||
         address.rfind("/local/", 0) == 0;
}

// The tile: one photo over the whole card, its caption in a soft band at the bottom. Home Assistant's slideshow sensor
// says which photo (its state is the address); a change slides the old one away and the new one in. A tap presses the
// sensor's next_button, so Home Assistant picks another photo at once.
class SlideshowTile : public tessera::Tile {
 public:
  explicit SlideshowTile(const tessera::Plugin *plugin) : plugin_(plugin) {}

  void create(const tessera::TileContext &c) override {
    width_ = c.width;
    height_ = c.height;
    contain_ = std::string(c.options["fit"] | "cover") == "contain";
    caption_on_ = c.options["caption"] | true;
    slide_ = c.options["slide"] | true;
    // Seconds between photos as this screen counts them; 0 leaves the pace to Home Assistant.
    const int every = c.options["interval"] | 0;
    interval_ = every > 0 ? static_cast<uint32_t>(std::max(every, 5)) : 0;

    if (ui::pictures()) {
      place_ = ui::picture(c.parent);
      lv_obj_set_size(place_, width_, height_);
      lv_obj_set_pos(place_, 0, 0);
      // The caption's band: the card's colour, a little see-through, over the photo.
      band_ = ui::block(c.parent, theme::CARD);
      lv_obj_set_style_bg_opa(band_, LV_OPA_70, 0);
      caption_ = ui::label(c.parent, Font::BODY, theme::INK);
      show_caption_("");
    }
    // What the tile says while there is no photo to show (or the board shows no pictures at all).
    message_ = ui::text(c.parent, Font::BODY, theme::MUTED);
    lv_obj_set_width(message_, width_ - 2 * ui::px(8));
    lv_obj_set_height(message_, 2 * ui::line_height(Font::BODY));
    lv_obj_set_style_text_align(message_, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_pos(message_, ui::px(8), (height_ - 2 * ui::line_height(Font::BODY)) / 2);
    ui::set_text(message_, place_ ? plugin_->text("choose") : plugin_->text("no_pictures"));
  }

  void on_state(JsonObjectConst data) override {
    if (data["state"].isNull()) {   // no sensor chosen yet in the editor
      wanted_ = "";
      wait_ = "choose";
    } else {
      const std::string state = data["state"] | "";
      wanted_ = is_photo(state) ? roomy::text(state) : tessera::Text();
      wait_ = wanted_.empty() ? "no_photo" : "";
    }
    caption_text_ = data["attributes"]["caption"] | "";
    next_button_ = data["attributes"]["next_button"] | "";
  }

  // The change of photo happens here, in a call of the tile, so the slide goes with it.
  void on_tick(uint32_t epoch) override {
    if (!place_) return;
    if (!(wanted_ == shown_)) {
      if (slide_ && !shown_.empty() && !slid_) ui::slide_pictures(-width_, 400);
      ui::set_picture(place_, roomy::plain(wanted_), contain_ ? tessera::Fit::CONTAIN : tessera::Fit::COVER);
      shown_ = wanted_;
      slid_ = false;
      since_ = epoch;   // every new photo, from the clock, a tap or Home Assistant, starts the count again
    }
    // The tile's own pace: after `interval_` seconds on the glass, the next photo, as a tap asks for it. Counted only
    // while the card is on the glass (on_tick runs then), and again from now when the ask did not go out.
    if (interval_ && epoch && !shown_.empty()) {
      if (!since_ || epoch < since_) since_ = epoch;
      if (epoch - since_ >= interval_) {
        press_next_();
        since_ = epoch;
      }
    }
    ui::set_text(message_, wait_.empty() ? "" : plugin_->text(wait_.c_str()));
    show_caption_(wanted_.empty() || !caption_on_ ? "" : roomy::plain(caption_text_));
  }

  void on_tap() override {
    if (press_next_() && place_ && slide_ && !shown_.empty() && !slid_) {
      ui::slide_pictures(-width_, 300);   // away at once; the next photo comes in when Home Assistant has picked it
      slid_ = true;
    }
  }

  void on_theme() override {
    ui::set_color(message_, theme::MUTED);
    if (caption_) ui::set_color(caption_, theme::INK);
    if (band_) ui::set_fill(band_, tessera::Tone::NORMAL, theme::CARD);
  }

 private:
  // Presses the sensor's next_button: Home Assistant picks another photo. False when there is none, or it did not go.
  bool press_next_() {
    const std::string button = roomy::plain(next_button_);
    if (button.rfind("input_button.", 0) != 0) return false;
    return tessera::action("input_button.press", button);
  }

  // The caption in its band at the bottom left, as wide as its words; nothing at all for no words.
  void show_caption_(const std::string &words) {
    if (!caption_) return;
    const int pad = ui::px(6), line = ui::line_height(Font::BODY);
    const int room = width_ - 2 * pad;
    const int w = words.empty() ? 0 : std::min(ui::text_width(words, Font::BODY), room - 2 * pad);
    ui::set_text(caption_, words);
    lv_obj_set_width(caption_, w > 0 ? w : 1);
    lv_obj_set_pos(caption_, 2 * pad, height_ - pad - line);
    if (words.empty()) {
      lv_obj_add_flag(band_, LV_OBJ_FLAG_HIDDEN);
    } else {
      lv_obj_clear_flag(band_, LV_OBJ_FLAG_HIDDEN);
      lv_obj_set_size(band_, w + 2 * pad, line + pad);
      lv_obj_set_pos(band_, pad, height_ - pad - line - pad / 2);
    }
  }

  const tessera::Plugin *plugin_;
  int width_ = 0, height_ = 0;
  bool contain_ = false, caption_on_ = true, slide_ = true, slid_ = false;
  uint32_t interval_ = 0, since_ = 0;
  tessera::Text wanted_, shown_, caption_text_, next_button_, wait_;   // PSRAM first (docs/FIRMWARE_API.md, "Memory")
  lv_obj_t *place_{}, *band_{}, *caption_{}, *message_{};
};

void PhotoSlideshow::setup() {
  add_tile("slideshow", [this]() { return new SlideshowTile(this); });
}

}  // namespace esphome::photo_slideshow
