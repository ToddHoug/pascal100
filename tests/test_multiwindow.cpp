#include "test.h"

#include "host/display_null.h"

#include <cstdint>
#include <string>

using namespace altair;

// ONE WINDOW PER VIDEO BOARD (issue #234). Before this, a Display kept a single Surface, so
// two video boards -- and especially two of the SAME resolution -- clobbered each other onto
// one buffer: whoever drew last won, and the other board's picture was gone. The seam now
// carries the drawing board's Owner handle (its `this`), and every host (the SDL back end's
// window, the NullDisplay's Surface) is kept one-per-owner. These tests prove the buffers no
// longer collide, headless, with no window (DESIGN.md 7.4).

void test_multiwindow() {
    SECTION("multi-window -- two owners at the SAME resolution get distinct surfaces");
    {
        // The heart of #234, at the seam itself: two owners ask for an identical (w,h,fmt).
        // The old single-surface Display handed both the same buffer; each must now get its
        // own. Opaque owner handles -- the Display never dereferences them, so two local
        // objects' addresses stand in for two boards.
        NullDisplay disp;
        char tagA = 0, tagB = 0;
        Display::Owner a = &tagA, b = &tagB;

        Surface* sa = disp.acquire(a, "vdmA", 512, 208, PixelFormat::Indexed8, 0);
        Surface* sb = disp.acquire(b, "vdmB", 512, 208, PixelFormat::Indexed8, 0);
        CHECK(sa != nullptr && sb != nullptr, "both owners get a surface");
        CHECK(sa != sb, "and they are DISTINCT buffers, not the one shared surface");
        CHECK(disp.surface(a) == sa && disp.surface(b) == sb,
              "each owner reads back its own surface");

        // Paint a signature into each and confirm the other is untouched -- proof they are
        // not aliases of one buffer.
        sa->clear(1);
        sb->clear(2);
        CHECK(disp.surface(a)->pixels()[0] == 1, "owner A keeps what A drew");
        CHECK(disp.surface(b)->pixels()[0] == 2, "owner B keeps what B drew, undisturbed by A");
    }

    SECTION("multi-window -- an owner that asks for another format gets a surface of that format");
    {
        NullDisplay disp;
        char tag = 0;
        Display::Owner a = &tag;
        Surface* s8 = disp.acquire(a, "a", 16, 8, PixelFormat::Indexed8, 0);
        CHECK(s8 && s8->pixels().size() == 128, "Indexed8: a byte a pixel");
        Surface* s32 = disp.acquire(a, "a", 16, 8, PixelFormat::Rgb32, 0);
        CHECK(s32 && s32->format() == PixelFormat::Rgb32 && s32->pixels().size() == 512 && s32->pitch() == 64,
              "Rgb32 at the same size: four bytes a pixel, the buffer replaced");
        CHECK(disp.acquire(a, "a", 16, 8, PixelFormat::Rgb32, 0) == s32, "and kept while the format holds");
    }

    SECTION("multi-window -- frames and palette are counted per owner, not pooled");
    {
        NullDisplay disp;
        char tagA = 0, tagB = 0;
        Display::Owner a = &tagA, b = &tagB;

        Surface* sa = disp.acquire(a, "a", 64, 64, PixelFormat::Indexed8, 0);
        Surface* sb = disp.acquire(b, "b", 64, 64, PixelFormat::Indexed8, 0);

        disp.present(a, sa);
        disp.present(a, sa);
        disp.present(b, sb);
        CHECK(disp.frames(a) == 2, "owner A's presents count only A's");
        CHECK(disp.frames(b) == 1, "owner B's count only B's");

        const Color redA[1]  = {{0xFF, 0, 0, 0xFF}};
        const Color blueB[1] = {{0, 0, 0xFF, 0xFF}};
        disp.setPalette(a, redA);
        disp.setPalette(b, blueB);
        CHECK(disp.palette(a).size() == 1 && disp.palette(a)[0].r == 0xFF,
              "owner A keeps its own palette");
        CHECK(disp.palette(b).size() == 1 && disp.palette(b)[0].b == 0xFF,
              "owner B keeps its own, unmerged with A's");
    }

    // pascal100 2026-11-09: the section that put a VDM-1 and a Flashwriter II in one machine
    // went with those boards. The display seam above stays: the serial `terminal` endpoint
    // still draws through it.
}
