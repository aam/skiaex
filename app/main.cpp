#include "SkCanvas.h"
#include "SkDocument.h"
#include "SkFontMgr.h"
#include "SkGradientShader.h"
#include "SkPaint.h"
#include "SkStream.h"
#include "SkString.h"
#include "SkTextBlob.h"
#include "SkTypeface.h"
#include <hb.h>
#include <hb-ft.h>

#include <iostream>
#include <map>

struct BaseOption {
  std::string selector;
  std::string description;
  virtual void set(std::string _value) = 0;
  virtual std::string valueToString() = 0;

  BaseOption(std::string _selector, std::string _description) :
    selector(_selector),
    description(_description) {}
};

template <class T> struct Option : BaseOption {
  T value;
  Option(std::string selector, std::string description, T defaultValue) :
    BaseOption(selector, description),
    value(defaultValue) {}
};

struct DoubleOption : Option<double> {
  virtual void set(std::string _value) {
    value = atof(_value.c_str());
  }
  virtual std::string valueToString() {
    return std::to_string(value);
  }
  DoubleOption(std::string selector, std::string description, double defaultValue) :
    Option<double>(selector, description, defaultValue) {}
};

struct SkStringOption : Option<SkString> {
  virtual void set(std::string _value) {
    value = _value.c_str();
  }
  virtual std::string valueToString() {
    return value.c_str();
  }
  SkStringOption(std::string selector, std::string description, SkString defaultValue) :
    Option<SkString>(selector, description, defaultValue) {}
};

struct StdStringOption : Option<std::string> {
  virtual void set(std::string _value) {
    value = _value;
  }
  virtual std::string valueToString() {
    return value;
  }
  StdStringOption(std::string selector, std::string description, std::string defaultValue) :
    Option<std::string>(selector, description, defaultValue) {}
};

struct Config {
  DoubleOption *page_width = new DoubleOption("-w", "Page width", 600.0f);
  DoubleOption *page_height = new DoubleOption("-h", "Page height", 800.0f);
  SkStringOption *title = new SkStringOption("-t", "PDF title", SkString("---"));
  SkStringOption *author = new SkStringOption("-a", "PDF author", SkString("---"));
  SkStringOption *subject = new SkStringOption("-k", "PDF subject", SkString("---"));
  SkStringOption *keywords = new SkStringOption("-c", "PDF keywords", SkString("---"));
  SkStringOption *creator = new SkStringOption("-t", "PDF creator", SkString("---"));
  StdStringOption *font_file = new StdStringOption("-f", ".ttf font file", "fonts/DejaVuSans.ttf");
  DoubleOption *font_size = new DoubleOption("-z", "Font size", 8.0f);
  DoubleOption *left_margin = new DoubleOption("-m", "Page height", 20.0f);
  DoubleOption *line_spacing_ratio = new DoubleOption("-h", "Line spacing ratio", 1.5f);
  StdStringOption *output_file_name = new StdStringOption("-o", ".pdf output file name", "out-skiahf.pdf");

  std::map<std::string, BaseOption*> options = {
    { page_width->selector, page_width },
    { page_height->selector, page_height },
    { title->selector, title },
    { author->selector, author },
    { subject->selector, subject },
    { keywords->selector, keywords },
    { creator->selector, creator },
    { font_file->selector, font_file },
    { font_size->selector, font_size },
    { left_margin->selector, left_margin },
    { line_spacing_ratio->selector, line_spacing_ratio },
    { output_file_name->selector, output_file_name },
  };

  Config(int argc, char **argv) {
    for (int i = 1; i < argc; i++) {
      std::string option_selector(argv[i]);
      auto it = options.find(option_selector);
      if (it != options.end()) {
        if (i >= argc) {
          break;
        }
        const char *option_value = argv[i + 1];
        it->second->set(option_value);
        i++;
      } else {
        printf("Ignoring unrecognized option: %s.\n", argv[i]);
        printf("Usage: %s {option value}\n", argv[0]);
        printf("Supported options:\n");
        for (auto it = options.begin(); it != options.end(); ++it) {
          printf("\t%s\t%s (%s)\n", it->first.c_str(),
            it->second->description.c_str(),
            it->second->valueToString().c_str());
        }
        exit(-1);
      }
    }
  } // end of Config::Config
};

const double FONT_SIZE_SCALE = 64.0f;

class Placement {
 public:
  Placement(Config &_config, SkWStream* outputStream) : config(_config),
    pdfDocument(SkDocument::CreatePDF(outputStream)) {
    FT_Error ft_error;

    if ((ft_error = FT_Init_FreeType (&ft_library)))
      abort();
    if ((ft_error = FT_New_Face (ft_library, config.font_file->value.c_str(), 0 /* face_index */, &ft_face)))
      abort();
    if ((ft_error = FT_Set_Char_Size (ft_face, config.font_size->value * FONT_SIZE_SCALE /* char_width */,
        config.font_size->value * FONT_SIZE_SCALE /* char_height */, 0 /* horz_resolution */, 0 /* vert_resolution */)))
      abort();
    hb_font = hb_ft_font_create (ft_face, NULL);

    typedef SkDocument::Attribute Attr;
    Attr pdf_info[] = {
        Attr(SkString("Title"),    config.title->value),
        Attr(SkString("Author"),   config.author->value),
        Attr(SkString("Subject"),  config.subject->value),
        Attr(SkString("Keywords"), config.keywords->value),
        Attr(SkString("Creator"),  config.creator->value),
    };
    int pdf_infoCount = sizeof(pdf_info) / sizeof(pdf_info[0]);
    std::unique_ptr<SkTime::DateTime> now = GetCurrentDateTime();

    pdfDocument->setMetadata(pdf_info, pdf_infoCount, now.get(), now.get());

    white_paint.setColor(SK_ColorWHITE);

    glyph_paint.setFlags(
        SkPaint::kAntiAlias_Flag |
        SkPaint::kSubpixelText_Flag);  // ... avoid waggly text when rotating.
    glyph_paint.setColor(SK_ColorBLACK);
    glyph_paint.setTextSize(config.font_size->value);
    SkAutoTUnref<SkFontMgr> fm(SkFontMgr::RefDefault());
    SkTypeface *typeface = SkTypeface::CreateFromFile(config.font_file->value.c_str(), 0);
    glyph_paint.setTypeface(typeface);
    glyph_paint.setTextEncoding(SkPaint::kGlyphID_TextEncoding);

    NewPage();
  } // end of Placement

  ~Placement() {
    hb_font_destroy (hb_font);

    FT_Done_Face (ft_face);
    FT_Done_FreeType (ft_library);
  }

  void WriteLine(const char *text) {
    /* Create hb-buffer and populate. */
    hb_buffer_t *hb_buffer = hb_buffer_create ();
    hb_buffer_add_utf8 (hb_buffer, text, -1, 0, -1);
    hb_buffer_guess_segment_properties (hb_buffer);

    /* Shape it! */
    hb_shape (hb_font, hb_buffer, NULL, 0);

    DrawGlyphs(hb_buffer);

    hb_buffer_destroy (hb_buffer);

    // Advance to the next line.
    current_y += config.line_spacing_ratio->value * config.font_size->value;
    if (current_y > config.page_height->value) {
      pdfDocument->endPage();
      NewPage();
    }
  }

  bool Close() {
    return pdfDocument->close();
  }

private:
  Config config;

  FT_Library ft_library;
  FT_Face ft_face;
  hb_font_t *hb_font;

  sk_sp<SkDocument> pdfDocument;

  SkCanvas* pageCanvas;

  SkPaint white_paint;
  SkPaint glyph_paint;

  double current_x;
  double current_y;

  void NewPage() {
    pageCanvas = pdfDocument->beginPage(config.page_width->value, config.page_height->value);

    pageCanvas->drawPaint(white_paint);

    current_x = config.left_margin->value;
    current_y = config.line_spacing_ratio->value * config.font_size->value;
  }

  bool DrawGlyphs(hb_buffer_t *hb_buffer) {
    SkTextBlobBuilder textBlobBuilder;
    unsigned len = hb_buffer_get_length (hb_buffer);
    hb_glyph_info_t *info = hb_buffer_get_glyph_infos (hb_buffer, NULL);
    hb_glyph_position_t *pos = hb_buffer_get_glyph_positions (hb_buffer, NULL);
    auto runBuffer = textBlobBuilder.allocRunPos(glyph_paint, len);

    double x = 0;
    double y = 0;
    for (unsigned int i = 0; i < len; i++)
    {
      runBuffer.glyphs[i] = info[i].codepoint;
      reinterpret_cast<SkPoint*>(runBuffer.pos)[i] = SkPoint::Make(
        x + pos[i].x_offset / FONT_SIZE_SCALE,
        y - pos[i].y_offset / FONT_SIZE_SCALE);
      x += pos[i].x_advance / FONT_SIZE_SCALE;
      y += pos[i].y_advance / FONT_SIZE_SCALE;
    }

    pageCanvas->drawTextBlob(textBlobBuilder.build(), current_x, current_y, glyph_paint);
  } // end of DrawGlyphs

  std::unique_ptr<SkTime::DateTime> GetCurrentDateTime() {
    time_t m_time;
    time(&m_time);
    struct tm* tstruct;
    tstruct = gmtime(&m_time);

    std::unique_ptr<SkTime::DateTime> now(new SkTime::DateTime());
    now->fTimeZoneMinutes = 0;
    now->fYear       = tstruct->tm_year + 1900;
    now->fMonth      = SkToU8(tstruct->tm_mon + 1);
    now->fDayOfWeek  = SkToU8(tstruct->tm_wday);
    now->fDay        = SkToU8(tstruct->tm_mday);
    now->fHour       = SkToU8(tstruct->tm_hour);
    now->fMinute     = SkToU8(tstruct->tm_min);
    now->fSecond     = SkToU8(tstruct->tm_sec);
    return now;
  }
}; // end of Placement class

int main(int argc, char** argv) {
    Config config(argc, argv);

    Placement placement(config, new SkFILEWStream(config.output_file_name->value.c_str()));
    for (std::string line; std::getline(std::cin, line);) {
      placement.WriteLine(line.c_str());
    }
    placement.Close();

    return 0;
}
