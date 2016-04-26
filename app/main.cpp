#include "SkCanvas.h"
#include "SkDocument.h"
#include "SkGradientShader.h"
#include "SkPaint.h"
#include "SkStream.h"
#include "SkString.h"

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

    return WritePDF(new SkFILEWStream("out.pdf")) ? 0 : 1;
}
