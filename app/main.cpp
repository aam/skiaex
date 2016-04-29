#include "SkCanvas.h"
#include "SkDocument.h"
#include "SkFontMgr.h"
#include "SkGradientShader.h"
#include "SkPaint.h"
#include "SkStream.h"
#include "SkString.h"
#include "SkTypeface.h"
#include <hb.h>
#include <hb-ft.h>
#include <cairo.h>
#include <cairo-ft.h>

struct Config {
  double page_width;
  double page_height;

  SkString title;
  SkString author;
  SkString subject;
  SkString keywords;
  SkString creator;

  std::string font_file;
  double font_size;

  Config(int argc, char **argv) :
    page_width(800.f), page_height(600.f),
    title("..."), author("..."), subject("..."), keywords("..."), creator("..."),
    font_file("fonts/NotoNastaliqUrdu-Regular.ttf"), font_size(24.0f) {
    for (int i = 1; i < argc; i++) {
      printf("Config arg[%d]=%s\n", argc, argv[i]);
      if (*argv[i] == '-') {
        char option_char = *(argv[i] + 1);
        if (i >= argc) {
          break;
        }
        const char *option_value = argv[i + 1];
        printf("\toption_char=%c\n", option_char);
        printf("\toption_value=%s\n", option_value);
        switch (option_char) {
          case 'w':
            page_width = atof(option_value);
            i++;
            break;
          case 'h':
            page_height = atof(option_value);
            i++;
            break;
          case 't':
            title = option_value;
            i++;
            break;
          case 'a':
            author = option_value;
            i++;
            break;
          case 's':
            subject = option_value;
            i++;
            break;
          case 'k':
            keywords = option_value;
            i++;
            break;
          case 'c':
            creator = option_value;
            i++;
            break;

          case 'f':
            font_file = option_value;
            i++;
            break;
          case 'z':
            font_size = atof(option_value);
            i++;
            break;
          default:
            printf("Unrecognized option: %c. Skipping.\n", option_char);
        }
      }
    }

  } // end of Config::Config
};


class Placement {
  Config config;

  FT_Library ft_library;
  FT_Face ft_face;
  hb_font_t *hb_font;
  hb_buffer_t *hb_buffer;

  const char *text;
  unsigned len;

  hb_glyph_position_t *pos;
  hb_glyph_info_t *info;

 public:
  Placement(Config &_config, const char *_text) : config(_config) {
    text = _text;
    FT_Error ft_error;

    if ((ft_error = FT_Init_FreeType (&ft_library)))
      abort();
    if ((ft_error = FT_New_Face (ft_library, config.font_file.c_str(), 0, &ft_face)))
      abort();
    if ((ft_error = FT_Set_Char_Size (ft_face, config.font_size*64,
        config.font_size*64, 0, 0)))
      abort();

    /* Create hb-ft font. */
    hb_font = hb_ft_font_create (ft_face, NULL);

    /* Create hb-buffer and populate. */
    hb_buffer = hb_buffer_create ();
    hb_buffer_add_utf8 (hb_buffer, text, -1, 0, -1);
    hb_buffer_guess_segment_properties (hb_buffer);

    /* Shape it! */
    hb_shape (hb_font, hb_buffer, NULL, 0);

    dump_hb_buffer(hb_buffer);
  } // end of Placement

  ~Placement() {
    hb_buffer_destroy (hb_buffer);
    hb_font_destroy (hb_font);

    FT_Done_Face (ft_face);
    FT_Done_FreeType (ft_library);
  }

  void DrawGlyphsUsingCairo(double margin) {
    /* Draw, using cairo. */
    double width = 2 * margin;
    double height = 2 * margin;
    for (unsigned int i = 0; i < len; i++)
    {
      width  += pos[i].x_advance / 64.;
      height -= pos[i].y_advance / 64.;
    }
    if (HB_DIRECTION_IS_HORIZONTAL (hb_buffer_get_direction(hb_buffer)))
      height += config.font_size;
    else
      width  += config.font_size;

    /* Set up cairo surface. */
    cairo_surface_t *cairo_surface;
    cairo_surface = cairo_image_surface_create (CAIRO_FORMAT_ARGB32,
                            ceil(width),
                            ceil(height));
    cairo_t *cr;
    cr = cairo_create (cairo_surface);
    cairo_set_source_rgba (cr, 1., 1., 1., 1.);
    cairo_paint (cr);
    cairo_set_source_rgba (cr, 0., 0., 0., 1.);
    cairo_translate (cr, margin, margin);

    /* Set up cairo font face. */
    cairo_font_face_t *cairo_face;
    cairo_face = cairo_ft_font_face_create_for_ft_face (ft_face, 0);
    cairo_set_font_face (cr, cairo_face);
    cairo_set_font_size (cr, config.font_size);

    /* Set up baseline. */
    if (HB_DIRECTION_IS_HORIZONTAL (hb_buffer_get_direction(hb_buffer)))
    {
      cairo_font_extents_t font_extents;
      cairo_font_extents (cr, &font_extents);
      double baseline = (config.font_size - font_extents.height) * .5 + font_extents.ascent;
      cairo_translate (cr, 0, baseline);
    }
    else
    {
      cairo_translate (cr, config.font_size * .5, 0);
    }

    cairo_glyph_t *cairo_glyphs = cairo_glyph_allocate (len);
    double current_x = 0;
    double current_y = 0;
    for (unsigned int i = 0; i < len; i++)
    {
      cairo_glyphs[i].index = info[i].codepoint;
      cairo_glyphs[i].x = current_x + pos[i].x_offset / 64.;
      cairo_glyphs[i].y = -(current_y + pos[i].y_offset / 64.);
      current_x += pos[i].x_advance / 64.;
      current_y += pos[i].y_advance / 64.;
    }
    cairo_show_glyphs (cr, cairo_glyphs, len);
    cairo_glyph_free (cairo_glyphs);

    cairo_surface_write_to_png (cairo_surface, "out-cairo.png");

    cairo_font_face_destroy (cairo_face);
    cairo_destroy (cr);
    cairo_surface_destroy (cairo_surface);
  } // end of DrawGlyphsUsingCairo

  bool DrawGlyphsUsingSkia(SkWStream* outputStream) {
    sk_sp<SkDocument> pdfDocument(SkDocument::CreatePDF(outputStream));
    typedef SkDocument::Attribute Attr;
    Attr pdf_info[] = {
        Attr(SkString("Title"),    config.title),
        Attr(SkString("Author"),   config.author),
        Attr(SkString("Subject"),  config.subject),
        Attr(SkString("Keywords"), config.keywords),
        Attr(SkString("Creator"),  config.creator),
    };
    int pdf_infoCount = sizeof(pdf_info) / sizeof(pdf_info[0]);
    std::unique_ptr<SkTime::DateTime> now = GetCurrentDateTime();

    pdfDocument->setMetadata(pdf_info, pdf_infoCount, now.get(), now.get());

    int numberOfPages = 1;
    for (int page = 0; page < numberOfPages; ++page) {
      SkScalar pageWidth = SkIntToScalar(800);
      SkScalar pageHeight = SkIntToScalar(600);
      SkCanvas* pageCanvas = pdfDocument->beginPage(pageWidth, pageHeight);

      SkScalar s = SkIntToScalar(600);
      static const SkPoint     kPts0[] = { { 0, 0 }, { s, s } };
      static const SkScalar    kPos[] = { 0, SK_Scalar1/2, SK_Scalar1 };
      static const SkColor kColors0[] = {0x80F00080, 0xF0F08000, 0x800080F0 };

      SkPaint paint;
      paint.setShader(SkGradientShader::MakeLinear(
          kPts0, kColors0, kPos, SK_ARRAY_COUNT(kColors0),
          SkShader::kClamp_TileMode));
      pageCanvas->drawPaint(paint);

      SkPaint textPaint;
      // Draw a message with a nice black paint.
      textPaint.setFlags(
          SkPaint::kAntiAlias_Flag |
          SkPaint::kSubpixelText_Flag | // ... avoid waggly text when rotating.
          SkPaint::kUnderlineText_Flag);
      textPaint.setColor(SK_ColorBLACK);
      textPaint.setTextSize(config.font_size);
      pageCanvas->save();
      pageCanvas->translate(40, 20);
      pageCanvas->rotate(45.0f);
      SkAutoTUnref<SkFontMgr> fm(SkFontMgr::RefDefault());
      SkTypeface * typeface = SkTypeface::CreateFromFile(config.font_file.c_str(), 0);
      textPaint.setTypeface(typeface);

      pageCanvas->drawText(text, strlen(text), 0, 0, textPaint);

      SkPoint *skpos = (SkPoint*)malloc(sizeof(SkPoint) * len);

      double current_x = 10;
      double current_y = 50;

      SkPaint glyphPaint(textPaint);
      glyphPaint.setTextEncoding(SkPaint::kGlyphID_TextEncoding);
      SkAutoSTMalloc<128, uint16_t> glyphStorage(len);
      uint16_t* glyphs = glyphStorage.get();
      for (unsigned int i = 0; i < len; i++)
      {
        glyphs[i] = info[i].codepoint;
        skpos[i] = SkPoint::Make(
          current_x + pos[i].x_offset / 64.,
          current_y - pos[i].y_offset / 64.);
        current_x += pos[i].x_advance / 64.;
        current_y += pos[i].y_advance / 64.;
      }

      pageCanvas->drawPosText(glyphs, len * sizeof(uint16_t), skpos, glyphPaint);

      current_x += 50;
      current_y += 50;

      SkTextBlobBuilder textBlobBuilder;
      auto runBuffer = textBlobBuilder.allocRunPos(glyphPaint, len);
      for (unsigned int i = 0; i < len; i++)
      {
        runBuffer.glyphs[i] = info[i].codepoint;
        skpos[i] = SkPoint::Make(
          current_x + pos[i].x_offset / 64.,
          current_y - pos[i].y_offset / 64.);
        current_x += pos[i].x_advance / 64.;
        current_y += pos[i].y_advance / 64.;
      }

      pageCanvas->drawTextBlob(textBlobBuilder.build(), x, y, glyphPaint);

      pageCanvas->restore();

      free(skpos);

      pdfDocument->endPage();
    }
    return pdfDocument->close();
  } // end of DrawGlyphsUsingSkia

private:
  void dump_hb_buffer(hb_buffer_t *hb_buffer) {
    /* Get glyph information and positions out of the buffer. */
    len = hb_buffer_get_length (hb_buffer);
    info = hb_buffer_get_glyph_infos (hb_buffer, NULL);
    pos = hb_buffer_get_glyph_positions (hb_buffer, NULL);

    /* Print them out as is. */
    printf ("Raw buffer contents:\n");
    for (unsigned int i = 0; i < len; i++)
    {
      hb_codepoint_t gid   = info[i].codepoint;
      unsigned int cluster = info[i].cluster;
      double x_advance = pos[i].x_advance / 64.;
      double y_advance = pos[i].y_advance / 64.;
      double x_offset  = pos[i].x_offset / 64.;
      double y_offset  = pos[i].y_offset / 64.;

      char glyphname[32];
      hb_font_get_glyph_name (hb_font, gid, glyphname, sizeof (glyphname));

      printf ("glyph='%s' cluster=%d  advance=(%g,%g) offset=(%g,%g)\n",
              glyphname, cluster, x_advance, y_advance, x_offset, y_offset);
    }
  }

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

    SkPaint paint;
    paint.setColor(SK_ColorRED);

    SkString* str = new SkString();
    paint.toString(str);

    fprintf(stdout, "%s\n", str->c_str());

    // CreateAndPlaceGlyphs(36, 36 * .5, "fonts/TSCu_SaiIndira.ttf", "டஉடு");
    // CreateAndPlaceGlyphs(36, 36 * .5, "fonts/DejaVuSans.ttf", "حرف‌باز");
    // Placement placement("fonts/NotoNastaliqUrdu-Regular.ttf", "حرف‌باز Привет, Вася! Hello, world! டஉடு", 24 /* font_size */);
    Placement placement(config, "حرف‌باز");
    placement.DrawGlyphsUsingCairo(24 * .5 /* margin */);
    placement.DrawGlyphsUsingSkia(new SkFILEWStream("out-skiahf.pdf"));

    return 0;
}
