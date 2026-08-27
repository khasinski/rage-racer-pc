#!/usr/bin/env python3
"""Regression tests for renderer-neutral sky panorama extraction."""

from extract import build_sky_panorama


def pixel(image, width, x, y):
    offset = (y * width + x) * 4
    return tuple(image[offset:offset + 4])


def test_tile_order_and_palette_independence():
    page = bytearray(256 * 256 * 4)
    for tile in range(8):
        source_x = (tile % 4) * 64
        source_y = (tile // 4) * 128
        color = (10 + tile, 40 + tile, 80 + tile, 255)
        for y in range(source_y, source_y + 128):
            for x in range(source_x, source_x + 64):
                offset = (y * 256 + x) * 4
                page[offset:offset + 4] = bytes(color)
    panorama = build_sky_panorama(page)
    assert len(panorama) == 512 * 128 * 4
    for tile in range(8):
        brightness = 80 + tile
        assert pixel(panorama, 512, tile * 64 + 31, 63) == (
            brightness, brightness, brightness, 255)


def test_transparent_background_stays_transparent():
    page = bytearray(256 * 256 * 4)
    page[0:4] = bytes((200, 20, 10, 0))
    panorama = build_sky_panorama(page)
    assert pixel(panorama, 512, 0, 0) == (0, 0, 0, 0)


if __name__ == "__main__":
    test_tile_order_and_palette_independence()
    test_transparent_background_stays_transparent()
