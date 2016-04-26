#include "SkCanvas.h"
#include "SkDocument.h"
#include "SkGradientShader.h"
#include "SkPaint.h"
#include "SkStream.h"
#include "SkString.h"
#include <hb.h>
#include <hb-ft.h>
#include <cairo.h>
#include <cairo-ft.h>

void CreateAndPlaceGlyphs(double fontSize, double margin, const char *fontfile, const char *text) {
  FT_Library ft_library;
  FT_Face ft_face;
  FT_Error ft_error;

  if ((ft_error = FT_Init_FreeType (&ft_library)))
    abort();
  if ((ft_error = FT_New_Face (ft_library, fontfile, 0, &ft_face)))
    abort();
  if ((ft_error = FT_Set_Char_Size (ft_face, fontSize*64, fontSize*64, 0, 0)))
    abort();

  /* Create hb-ft font. */
  hb_font_t *hb_font;
  hb_font = hb_ft_font_create (ft_face, NULL);

  /* Create hb-buffer and populate. */
  hb_buffer_t *hb_buffer;
  hb_buffer = hb_buffer_create ();
  hb_buffer_add_utf8 (hb_buffer, text, -1, 0, -1);
  hb_buffer_guess_segment_properties (hb_buffer);

  /* Shape it! */
  hb_shape (hb_font, hb_buffer, NULL, 0);

  /* Get glyph information and positions out of the buffer. */
  unsigned int len = hb_buffer_get_length (hb_buffer);
  hb_glyph_info_t *info = hb_buffer_get_glyph_infos (hb_buffer, NULL);
  hb_glyph_position_t *pos = hb_buffer_get_glyph_positions (hb_buffer, NULL);

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

  printf ("Converted to absolute positions:\n");
  /* And converted to absolute positions. */
  {
    double current_x = 0;
    double current_y = 0;
    for (unsigned int i = 0; i < len; i++)
    {
      hb_codepoint_t gid   = info[i].codepoint;
      unsigned int cluster = info[i].cluster;
      double x_position = current_x + pos[i].x_offset / 64.;
      double y_position = current_y + pos[i].y_offset / 64.;


      char glyphname[32];
      hb_font_get_glyph_name (hb_font, gid, glyphname, sizeof (glyphname));

      printf ("glyph='%s'   cluster=%d  position=(%g,%g)\n",
          glyphname, cluster, x_position, y_position);

      current_x += pos[i].x_advance / 64.;
      current_y += pos[i].y_advance / 64.;
    }
  }

  /* Draw, using cairo. */
  double width = 2 * margin;
  double height = 2 * margin;
  for (unsigned int i = 0; i < len; i++)
  {
    width  += pos[i].x_advance / 64.;
    height -= pos[i].y_advance / 64.;
  }
  if (HB_DIRECTION_IS_HORIZONTAL (hb_buffer_get_direction(hb_buffer)))
    height += fontSize;
  else
    width  += fontSize;

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
  cairo_set_font_size (cr, fontSize);

  /* Set up baseline. */
  if (HB_DIRECTION_IS_HORIZONTAL (hb_buffer_get_direction(hb_buffer)))
  {
    cairo_font_extents_t font_extents;
    cairo_font_extents (cr, &font_extents);
    double baseline = (fontSize - font_extents.height) * .5 + font_extents.ascent;
    cairo_translate (cr, 0, baseline);
  }
  else
  {
    cairo_translate (cr, fontSize * .5, 0);
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

  cairo_surface_write_to_png (cairo_surface, "out.png");

  cairo_font_face_destroy (cairo_face);
  cairo_destroy (cr);
  cairo_surface_destroy (cairo_surface);

  hb_buffer_destroy (hb_buffer);
  hb_font_destroy (hb_font);

  FT_Done_Face (ft_face);
  FT_Done_FreeType (ft_library);
}


bool WritePDF(SkWStream* outputStream) {
    sk_sp<SkDocument> pdfDocument(SkDocument::CreatePDF(outputStream));
    typedef SkDocument::Attribute Attr;
    Attr info[] = {
        Attr(SkString("Title"),    SkString("....")),
        Attr(SkString("Author"),   SkString("....")),
        Attr(SkString("Subject"),  SkString("....")),
        Attr(SkString("Keywords"), SkString("....")),
        Attr(SkString("Creator"),  SkString("....")),
    };
    int infoCount = sizeof(info) / sizeof(info[0]);
    SkTime::DateTime now;
    SkTime::GetDateTime(&now);
    pdfDocument->setMetadata(info, infoCount, &now, &now);

    int numberOfPages = 1;
    for (int page = 0; page < numberOfPages; ++page) {
        SkScalar pageWidth = SkIntToScalar(800);
        SkScalar pageHeight = SkIntToScalar(600);
        SkCanvas* pageCanvas =
                pdfDocument->beginPage(pageWidth, pageHeight);

        SkScalar s = SkIntToScalar(600);
        static const SkPoint     kPts0[] = { { 0, 0 }, { s, s } };
        static const SkScalar    kPos[] = { 0, SK_Scalar1/2, SK_Scalar1 };
        static const SkColor kColors0[] = {0x80F00080, 0xF0F08000, 0x800080F0 };

        SkPaint paint;
        paint.setShader(SkGradientShader::MakeLinear(
            kPts0, kColors0, kPos, SK_ARRAY_COUNT(kColors0),
            SkShader::kClamp_TileMode));
        pageCanvas->drawPaint(paint);

        pdfDocument->endPage();
    }
    return pdfDocument->close();
}

int main(int argc, char** argv) {
    SkPaint paint;
    paint.setColor(SK_ColorRED);

    SkString* str = new SkString();
    paint.toString(str);

    fprintf(stdout, "%s\n", str->c_str());

    CreateAndPlaceGlyphs(36, 36 * .5, "fonts/DejaVuSerif.ttf", "Привет, Вася!");

    return WritePDF(new SkFILEWStream("out.pdf")) ? 0 : 1;
}
