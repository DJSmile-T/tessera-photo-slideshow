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

// What Home Assistant's slideshow sensor says about the photo it picked (homeassistant/photo_slideshow.yaml): its state
// is the photo's address, its attributes the caption (album and which photo of how many), the photo's own name, where it
// was taken and when (photo_date, seconds since 1970: the app turns an attribute named ...date into a moment), and the
// button that asks for the next photo. The tile and the screensaver step read the same.
struct PhotoInfo {
  tessera::Text address, caption, name, place, next_button, wait;   // PSRAM first (docs/FIRMWARE_API.md, "Memory")
  uint32_t taken = 0;

  void read(JsonObjectConst data) {
    if (data["state"].isNull()) {   // no sensor chosen yet in the editor
      address = "";
      wait = "choose";
    } else {
      const std::string state = data["state"] | "";
      address = is_photo(state) ? roomy::text(state) : tessera::Text();
      wait = address.empty() ? "no_photo" : "";
    }
    JsonObjectConst a = data["attributes"];
    caption = a["caption"] | "";
    name = a["photo_name"] | "";
    place = a["place"] | "";
    next_button = a["next_button"] | "";
    taken = a["photo_date"].is<uint32_t>() ? a["photo_date"].as<uint32_t>() : 0;
  }

  // Presses the next_button: Home Assistant picks another photo. False when there is none, or it did not go.
  bool press_next() const {
    const std::string button = roomy::plain(next_button);
    if (button.rfind("input_button.", 0) != 0) return false;
    return tessera::action("input_button.press", button);
  }
};

// Which of the photo's details the person wants to see (the options of the tile and of the screensaver step).
struct Shown {
  bool caption = true, name = false, place = true, date = false, clock = false;
  bool right = false, top = false;

  void read(JsonObjectConst o, bool clock_default) {
    caption = o["caption"] | true;
    name = o["name"] | false;
    place = o["place"] | true;
    date = o["date"] | false;
    clock = o["time"] | clock_default;
    const std::string corner = o["corner"] | "bottom_left";
    right = corner.find("right") != std::string::npos;
    top = corner.rfind("top", 0) == 0;
  }

  // The second line: the photo's name, where and when, those the person chose and the photo has, a dot between two.
  std::string details(const PhotoInfo &p) const {
    std::string out;
    auto add = [&out](const std::string &part) {
      if (part.empty()) return;
      if (!out.empty()) out += " \xC2\xB7 ";   // a middle dot
      out += part;
    };
    if (name) add(roomy::plain(p.name));
    if (place) add(roomy::plain(p.place));
    if (date && p.taken) {
      const tessera::LocalTime t = tessera::local_time(p.taken);
      add(tessera::date_text(p.taken) + " " + std::to_string(t.year));
    }
    return out;
  }
};

// The words over the photo: up to three lines (the time, the caption, the details) on a soft band in a corner, as wide
// as the widest line. Paint only when a line changed; a band without words hides.
class InfoBand {
 public:
  void make(lv_obj_t *parent, int width, int height, Font time_font, theme::Role ground, lv_opa_t opacity, int edge,
            int pad) {
    width_ = width;
    height_ = height;
    time_font_ = time_font;
    edge_ = edge;
    pad_ = pad;
    band_ = ui::block(parent, ground);
    lv_obj_set_style_bg_opa(band_, opacity, 0);
    lv_obj_add_flag(band_, LV_OBJ_FLAG_HIDDEN);
    time_ = ui::label(band_, time_font, theme::INK);
    caption_ = ui::label(band_, Font::BODY, theme::INK);
    details_ = ui::label(band_, Font::BODY, theme::INK_SOFT);
  }

  void paint(const std::string &time, const std::string &caption, const std::string &details, bool right, bool top) {
    if (!band_) return;
    if (roomy::same(time, time_text_) && roomy::same(caption, caption_text_) && roomy::same(details, details_text_) &&
        right == right_ && top == top_)
      return;
    time_text_ = roomy::text(time);
    caption_text_ = roomy::text(caption);
    details_text_ = roomy::text(details);
    right_ = right;
    top_ = top;
    if (time.empty() && caption.empty() && details.empty()) {
      lv_obj_add_flag(band_, LV_OBJ_FLAG_HIDDEN);
      return;
    }
    const int room = std::max(width_ - 2 * edge_ - 2 * pad_, 1);
    const int th = time.empty() ? 0 : ui::line_height(time_font_);
    const int ch = caption.empty() ? 0 : ui::line_height(Font::BODY);
    const int dh = details.empty() ? 0 : ui::line_height(Font::BODY);
    const int tw = time.empty() ? 0 : std::min(ui::text_width(time, time_font_), room);
    const int cw = caption.empty() ? 0 : std::min(ui::text_width(caption, Font::BODY), room);
    const int dw = details.empty() ? 0 : std::min(ui::text_width(details, Font::BODY), room);
    const int w = std::max({tw, cw, dw}) + 2 * pad_, h = th + ch + dh + 2 * pad_;
    place_line_(time_, time, tw, w, pad_);
    place_line_(caption_, caption, cw, w, pad_ + th);
    place_line_(details_, details, dw, w, pad_ + th + ch);
    lv_obj_set_size(band_, w, h);
    lv_obj_set_pos(band_, right ? width_ - edge_ - w : edge_, top ? edge_ : height_ - edge_ - h);
    lv_obj_remove_flag(band_, LV_OBJ_FLAG_HIDDEN);
  }

  // Light and dark (the tile's on_theme): the roles have other values now.
  void recolour(theme::Role ground) {
    if (!band_) return;
    ui::set_fill(band_, tessera::Tone::NORMAL, ground);
    ui::set_color(time_, theme::INK);
    ui::set_color(caption_, theme::INK);
    ui::set_color(details_, theme::INK_SOFT);
  }

 private:
  void place_line_(lv_obj_t *label, const std::string &words, int w, int band_w, int y) {
    ui::set_text(label, words);
    if (words.empty()) {
      lv_obj_add_flag(label, LV_OBJ_FLAG_HIDDEN);
      return;
    }
    lv_obj_remove_flag(label, LV_OBJ_FLAG_HIDDEN);
    lv_obj_set_width(label, std::max(w, 1));
    lv_obj_set_pos(label, right_ ? band_w - pad_ - w : pad_, y);
  }

  int width_ = 0, height_ = 0, edge_ = 0, pad_ = 0;
  Font time_font_ = Font::TITLE;
  bool right_ = false, top_ = false;
  tessera::Text time_text_, caption_text_, details_text_;
  lv_obj_t *band_{}, *time_{}, *caption_{}, *details_{};
};

// The tile: one photo over the whole card, the details the person chose on a soft band in a corner. Home Assistant's
// slideshow sensor says which photo (its state is the address); a change slides the old one away and the new one in. A
// tap, and the tile's own seconds between photos, press the sensor's next_button, so Home Assistant picks another.
class SlideshowTile : public tessera::Tile {
 public:
  explicit SlideshowTile(const tessera::Plugin *plugin) : plugin_(plugin) {}

  void create(const tessera::TileContext &c) override {
    width_ = c.width;
    height_ = c.height;
    contain_ = std::string(c.options["fit"] | "cover") == "contain";
    slide_ = c.options["slide"] | true;
    shown_opts_.read(c.options, false);
    // Seconds between photos as this screen counts them; 0 leaves the pace to Home Assistant.
    const int every = c.options["interval"] | 0;
    interval_ = every > 0 ? static_cast<uint32_t>(std::max(every, 5)) : 0;

    if (ui::pictures()) {
      place_ = ui::picture(c.parent);
      lv_obj_set_size(place_, width_, height_);
      lv_obj_set_pos(place_, 0, 0);
      // The band: the card's colour, a little see-through, over the photo.
      band_.make(c.parent, width_, height_, Font::TITLE, theme::CARD, LV_OPA_70, ui::px(6), ui::px(6));
    }
    // What the tile says while there is no photo to show (or the board shows no pictures at all).
    message_ = ui::text(c.parent, Font::BODY, theme::MUTED);
    lv_obj_set_width(message_, width_ - 2 * ui::px(8));
    lv_obj_set_height(message_, 2 * ui::line_height(Font::BODY));
    lv_obj_set_style_text_align(message_, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_pos(message_, ui::px(8), (height_ - 2 * ui::line_height(Font::BODY)) / 2);
    ui::set_text(message_, place_ ? plugin_->text("choose") : plugin_->text("no_pictures"));
  }

  void on_state(JsonObjectConst data) override { photo_.read(data); }

  // The change of photo happens here, in a call of the tile, so the slide goes with it.
  void on_tick(uint32_t epoch) override {
    if (!place_) return;
    if (!(photo_.address == shown_)) {
      if (slide_ && !shown_.empty() && !slid_) ui::slide_pictures(-width_, 400);
      ui::set_picture(place_, roomy::plain(photo_.address), contain_ ? tessera::Fit::CONTAIN : tessera::Fit::COVER);
      shown_ = photo_.address;
      slid_ = false;
      since_ = epoch;   // every new photo, from the clock, a tap or Home Assistant, starts the count again
    }
    // The tile's own pace: after `interval_` seconds on the glass, the next photo, as a tap asks for it. Counted only
    // while the card is on the glass (on_tick runs then), and again from now when the ask did not go out.
    if (interval_ && epoch && !shown_.empty()) {
      if (!since_ || epoch < since_) since_ = epoch;
      if (epoch - since_ >= interval_) {
        photo_.press_next();
        since_ = epoch;
      }
    }
    ui::set_text(message_, photo_.wait.empty() ? "" : plugin_->text(photo_.wait.c_str()));
    const bool any = !photo_.address.empty();
    band_.paint(any && shown_opts_.clock && epoch ? tessera::clock_text(epoch) : "",
                any && shown_opts_.caption ? roomy::plain(photo_.caption) : "",
                any ? shown_opts_.details(photo_) : "", shown_opts_.right, shown_opts_.top);
  }

  void on_tap() override {
    if (photo_.press_next() && place_ && slide_ && !shown_.empty() && !slid_) {
      ui::slide_pictures(-width_, 300);   // away at once; the next photo comes in when Home Assistant has picked it
      slid_ = true;
    }
  }

  void on_theme() override {
    ui::set_color(message_, theme::MUTED);
    band_.recolour(theme::CARD);
  }

 private:
  const tessera::Plugin *plugin_;
  int width_ = 0, height_ = 0;
  bool contain_ = false, slide_ = true, slid_ = false;
  uint32_t interval_ = 0, since_ = 0;
  Shown shown_opts_;
  PhotoInfo photo_;
  tessera::Text shown_;
  InfoBand band_;
  lv_obj_t *place_{}, *message_{};
};

// A step of its own in the screensaver (plugin API 0.14): the photo of the slideshow sensor over the whole glass, with
// the details the person chose on a band in a corner, the time first by default. The same photos and the same details
// as the tile, in standby. With seconds between photos it asks Home Assistant for the next photo itself, as the tile.
class SlideshowSaver : public tessera::Saver {
 public:
  explicit SlideshowSaver(const tessera::Plugin *plugin) : plugin_(plugin) {}

  void show(const tessera::SaverContext &c) override {
    width_ = c.width;
    height_ = c.height;
    contain_ = std::string(c.options["fit"] | "cover") == "contain";
    shown_opts_.read(c.options, true);
    const int every = c.options["interval"] | 0;
    interval_ = every > 0 ? static_cast<uint32_t>(std::max(every, 5)) : 0;
    if (ui::pictures()) {
      place_ = ui::picture(c.parent);
      lv_obj_set_size(place_, width_, height_);
      lv_obj_set_pos(place_, 0, 0);
    }
    // While there is no photo (or on a board without pictures): a word in the middle.
    message_ = ui::text(c.parent, Font::BODY, theme::MUTED);
    lv_obj_set_width(message_, width_ - 2 * ui::mm(4));
    lv_obj_set_height(message_, 2 * ui::line_height(Font::BODY));
    lv_obj_set_style_text_align(message_, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_pos(message_, ui::mm(4), (height_ - 2 * ui::line_height(Font::BODY)) / 2);
    band_.make(c.parent, width_, height_, Font::HEADLINE, theme::CAMERA_PAGE, LV_OPA_50, ui::mm(4), ui::mm(2));
  }

  void on_state(JsonObjectConst data) override { photo_.read(data); }

  void on_tick(uint32_t epoch) override {
    if (place_ && !(photo_.address == shown_)) {
      ui::set_picture(place_, roomy::plain(photo_.address), contain_ ? tessera::Fit::CONTAIN : tessera::Fit::COVER);
      shown_ = photo_.address;
      since_ = epoch;
    }
    if (interval_ && epoch && !shown_.empty()) {
      if (!since_ || epoch < since_) since_ = epoch;
      if (epoch - since_ >= interval_) {
        photo_.press_next();
        since_ = epoch;
      }
    }
    const char *wait = !place_ ? "no_pictures" : photo_.wait.c_str();
    ui::set_text(message_, *wait ? plugin_->text(wait) : "");
    const bool any = !photo_.address.empty();
    band_.paint(shown_opts_.clock && epoch ? tessera::clock_text(epoch) : "",
                any && shown_opts_.caption ? roomy::plain(photo_.caption) : "",
                any ? shown_opts_.details(photo_) : "", shown_opts_.right, shown_opts_.top);
  }

 private:
  const tessera::Plugin *plugin_;
  int width_ = 0, height_ = 0;
  bool contain_ = false;
  uint32_t interval_ = 0, since_ = 0;
  Shown shown_opts_;
  PhotoInfo photo_;
  tessera::Text shown_;
  InfoBand band_;
  lv_obj_t *place_{}, *message_{};
};

// The face of the screensaver's photos (plugin API 0.14): Tessera's own Photos step picks the photo and shows it over
// the whole glass under this face (`backdrop: true`, moving slowly when the person chose Motion: Slow). The face draws
// what the tile draws over its photo: the caption, the album and which photo of how many, on a soft band in a corner,
// and the time with it when the person wants. The screen moves the band a millimetre every minute against burn-in.
class CaptionSaver : public tessera::Saver {
 public:
  void show(const tessera::SaverContext &c) override {
    width_ = c.width;
    height_ = c.height;
    // An option the person never touched may be missing: read each with its default.
    const char *corner = c.options["corner"] | "bottom_left";
    right_ = std::string(corner).find("right") != std::string::npos;
    top_ = std::string(corner).rfind("top", 0) == 0;
    clock_ = c.options["time"] | false;
    numbers_ = c.options["numbers"] | true;
    title_ = c.options["title"] | "";
    // The band, over the photo, and its two lines.
    band_ = ui::block(c.parent, theme::CAMERA_PAGE);
    lv_obj_set_style_bg_opa(band_, LV_OPA_50, 0);
    lv_obj_add_flag(band_, LV_OBJ_FLAG_HIDDEN);
    time_ = ui::label(band_, Font::HEADLINE, theme::INK);
    caption_ = ui::label(band_, Font::BODY, theme::INK_SOFT);
  }

  // The step's contract: picture, mark, index, count, album. Kept here, painted in on_tick.
  void on_state(JsonObjectConst data) override {
    album_ = data["album"] | "";
    index_ = data["index"] | 0;
    count_ = data["count"] | 0;
    dirty_ = true;
  }

  void on_tick(uint32_t epoch) override {
    const uint32_t minute = clock_ && epoch ? epoch / 60 : 0;
    if (!dirty_ && minute == minute_) return;   // nothing new: paint nothing
    dirty_ = false;
    minute_ = minute;
    std::string words = title_.empty() ? roomy::plain(album_) : roomy::plain(title_);
    if (numbers_ && index_ > 0 && count_ > 0) {
      if (!words.empty()) words += " \xC2\xB7 ";   // a middle dot between the album and the numbers
      words += std::to_string(index_) + "/" + std::to_string(count_);
    }
    const std::string time = minute ? tessera::clock_text(epoch) : "";
    if (words.empty() && time.empty()) {
      lv_obj_add_flag(band_, LV_OBJ_FLAG_HIDDEN);
      return;
    }
    const int pad = ui::mm(2), edge = ui::mm(4), room = width_ - 2 * edge - 2 * pad;
    const int th = time.empty() ? 0 : ui::line_height(Font::HEADLINE), ch = words.empty() ? 0 : ui::line_height(Font::BODY);
    const int tw = time.empty() ? 0 : std::min(ui::text_width(time, Font::HEADLINE), room);
    const int cw = words.empty() ? 0 : std::min(ui::text_width(words, Font::BODY), room);
    const int w = std::max(tw, cw) + 2 * pad, h = th + ch + 2 * pad;
    ui::set_text(time_, time);
    ui::set_text(caption_, words);
    lv_obj_set_width(time_, std::max(tw, 1));
    lv_obj_set_width(caption_, std::max(cw, 1));
    lv_obj_set_pos(time_, right_ ? w - pad - tw : pad, pad);
    lv_obj_set_pos(caption_, right_ ? w - pad - cw : pad, pad + th);
    lv_obj_set_size(band_, w, h);
    lv_obj_set_pos(band_, right_ ? width_ - edge - w : edge, top_ ? edge : height_ - edge - h);
    lv_obj_remove_flag(band_, LV_OBJ_FLAG_HIDDEN);
  }

 private:
  int width_ = 0, height_ = 0, index_ = 0, count_ = 0;
  bool right_ = false, top_ = false, clock_ = false, numbers_ = true, dirty_ = true;
  uint32_t minute_ = 0;
  tessera::Text album_, title_;   // PSRAM first (docs/FIRMWARE_API.md, "Memory")
  lv_obj_t *band_{}, *time_{}, *caption_{};
};

void PhotoSlideshow::setup() {
  add_tile("slideshow", [this]() { return new SlideshowTile(this); });
  // The face of Tessera's own Photos screensaver step (plugin API 0.14); its step must be the manifest's.
  add_saver("caption", tessera::SaverStep::PHOTOS, []() { return new CaptionSaver(); });
  // A step of its own (plugin API 0.14): the slideshow sensor's photo with every detail, in standby.
  add_saver("slideshow", tessera::SaverStep::OWN, [this]() { return new SlideshowSaver(this); });
}

}  // namespace esphome::photo_slideshow
