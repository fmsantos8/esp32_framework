#!/usr/bin/env python3
"""Generate packed font data with Pillow and a supplied Lato-Semibold.ttf file.

Usage: python3 tools/generate_fonts.py /path/to/Lato-Semibold.ttf
The generated C file is committed; firmware builds do not require Python/Pillow.
"""

import argparse
from pathlib import Path

from PIL import Image, ImageDraw, ImageFont


# Original 5x7 glyphs, written as top-to-bottom rows. Space and degree are narrower.
SMALL = {
    ' ': '000/000/000/000/000/000/000',
    '!': '00100/00100/00100/00100/00100/00000/00100',
    '"': '01010/01010/01010/00000/00000/00000/00000',
    '#': '01010/01010/11111/01010/11111/01010/01010',
    '$': '00100/01111/10100/01110/00101/11110/00100',
    '%': '11001/11010/00010/00100/01000/01011/10011',
    '&': '01100/10010/10100/01000/10101/10010/01101',
    "'": '00100/00100/01000/00000/00000/00000/00000',
    '(': '00010/00100/01000/01000/01000/00100/00010',
    ')': '01000/00100/00010/00010/00010/00100/01000',
    '*': '00000/00100/10101/01110/10101/00100/00000',
    '+': '00000/00100/00100/11111/00100/00100/00000',
    ',': '00000/00000/00000/00000/00110/00100/01000',
    '-': '00000/00000/00000/11111/00000/00000/00000',
    '.': '00000/00000/00000/00000/00000/00110/00110',
    '/': '00001/00010/00010/00100/01000/01000/10000',
    '0': '01110/10001/10011/10101/11001/10001/01110',
    '1': '00100/01100/00100/00100/00100/00100/01110',
    '2': '01110/10001/00001/00010/00100/01000/11111',
    '3': '11110/00001/00001/01110/00001/00001/11110',
    '4': '00010/00110/01010/10010/11111/00010/00010',
    '5': '11111/10000/10000/11110/00001/00001/11110',
    '6': '01110/10000/10000/11110/10001/10001/01110',
    '7': '11111/00001/00010/00100/01000/01000/01000',
    '8': '01110/10001/10001/01110/10001/10001/01110',
    '9': '01110/10001/10001/01111/00001/00001/01110',
    ':': '00000/00110/00110/00000/00110/00110/00000',
    ';': '00000/00110/00110/00000/00110/00100/01000',
    '<': '00010/00100/01000/10000/01000/00100/00010',
    '=': '00000/00000/11111/00000/11111/00000/00000',
    '>': '01000/00100/00010/00001/00010/00100/01000',
    '?': '01110/10001/00001/00010/00100/00000/00100',
    '@': '01110/10001/10111/10101/10111/10000/01110',
    'A': '01110/10001/10001/11111/10001/10001/10001',
    'B': '11110/10001/10001/11110/10001/10001/11110',
    'C': '01110/10001/10000/10000/10000/10001/01110',
    'D': '11110/10001/10001/10001/10001/10001/11110',
    'E': '11111/10000/10000/11110/10000/10000/11111',
    'F': '11111/10000/10000/11110/10000/10000/10000',
    'G': '01110/10001/10000/10111/10001/10001/01110',
    'H': '10001/10001/10001/11111/10001/10001/10001',
    'I': '11111/00100/00100/00100/00100/00100/11111',
    'J': '00111/00010/00010/00010/10010/10010/01100',
    'K': '10001/10010/10100/11000/10100/10010/10001',
    'L': '10000/10000/10000/10000/10000/10000/11111',
    'M': '10001/11011/10101/10101/10001/10001/10001',
    'N': '10001/11001/10101/10011/10001/10001/10001',
    'O': '01110/10001/10001/10001/10001/10001/01110',
    'P': '11110/10001/10001/11110/10000/10000/10000',
    'Q': '01110/10001/10001/10001/10101/10010/01101',
    'R': '11110/10001/10001/11110/10100/10010/10001',
    'S': '01111/10000/10000/01110/00001/00001/11110',
    'T': '11111/00100/00100/00100/00100/00100/00100',
    'U': '10001/10001/10001/10001/10001/10001/01110',
    'V': '10001/10001/10001/10001/10001/01010/00100',
    'W': '10001/10001/10001/10101/10101/10101/01010',
    'X': '10001/10001/01010/00100/01010/10001/10001',
    'Y': '10001/10001/01010/00100/00100/00100/00100',
    'Z': '11111/00001/00010/00100/01000/10000/11111',
    '[': '01110/01000/01000/01000/01000/01000/01110',
    '\\': '10000/01000/01000/00100/00010/00010/00001',
    ']': '01110/00010/00010/00010/00010/00010/01110',
    '^': '00100/01010/10001/00000/00000/00000/00000',
    '_': '00000/00000/00000/00000/00000/00000/11111',
    '`': '01000/00100/00010/00000/00000/00000/00000',
    'a': '00000/00000/01110/00001/01111/10001/01111',
    'b': '10000/10000/10110/11001/10001/10001/11110',
    'c': '00000/00000/01110/10000/10000/10001/01110',
    'd': '00001/00001/01101/10011/10001/10001/01111',
    'e': '00000/00000/01110/10001/11111/10000/01110',
    'f': '00110/01001/01000/11100/01000/01000/01000',
    'g': '00000/01111/10001/10001/01111/00001/01110',
    'h': '10000/10000/10110/11001/10001/10001/10001',
    'i': '00100/00000/01100/00100/00100/00100/01110',
    'j': '00010/00000/00110/00010/00010/10010/01100',
    'k': '10000/10000/10010/10100/11000/10100/10010',
    'l': '01100/00100/00100/00100/00100/00100/01110',
    'm': '00000/00000/11010/10101/10101/10101/10101',
    'n': '00000/00000/10110/11001/10001/10001/10001',
    'o': '00000/00000/01110/10001/10001/10001/01110',
    'p': '00000/11110/10001/10001/11110/10000/10000',
    'q': '00000/01111/10001/10001/01111/00001/00001',
    'r': '00000/00000/10110/11001/10000/10000/10000',
    's': '00000/00000/01111/10000/01110/00001/11110',
    't': '01000/01000/11100/01000/01000/01001/00110',
    'u': '00000/00000/10001/10001/10001/10011/01101',
    'v': '00000/00000/10001/10001/10001/01010/00100',
    'w': '00000/00000/10001/10001/10101/10101/01010',
    'x': '00000/00000/10001/01010/00100/01010/10001',
    'y': '00000/10001/10001/10001/01111/00001/01110',
    'z': '00000/00000/11111/00010/00100/01000/11111',
    '{': '00010/00100/00100/01000/00100/00100/00010',
    '|': '00100/00100/00100/00100/00100/00100/00100',
    '}': '01000/00100/00100/00010/00100/00100/01000',
    '~': '00000/00000/01001/10110/00000/00000/00000',
    '°': '111/101/111/000/000/000/000',
}


def small_image(character):
    rows = SMALL[character].split('/')
    assert len(rows) == 7 and all(len(row) == len(rows[0]) for row in rows)
    image = Image.new('1', (len(rows[0]), 7))
    image.putdata([int(pixel) for row in rows for pixel in row])
    return image


def proportional_image(character, font, height, baseline):
    left, top, right, bottom = font.getbbox(character, anchor='ls')
    assert top + baseline >= 0 and bottom + baseline <= height
    image = Image.new('L', (max(1, right - left), height))
    ImageDraw.Draw(image).text((-left, baseline), character, font=font, anchor='ls', fill=255)
    image = image.point(lambda pixel: 255 if pixel >= 128 else 0, '1')
    bounds = image.getbbox()
    if bounds is None:
        return Image.new('1', (max(1, round(font.getlength(character))), height))
    return image.crop((bounds[0], 0, bounds[2], height))


def pack(image):
    result = bytearray()
    for y in range(image.height):
        for start in range(0, image.width, 8):
            value = 0
            for bit in range(8):
                if start + bit < image.width and image.getpixel((start + bit, y)):
                    value |= 0x80 >> bit
            result.append(value)
    return result


def emit_font(name, characters, render):
    bitmap = bytearray()
    glyphs = []
    for character in characters:
        image = render(character)
        assert 0 < image.width <= 255
        glyphs.append((ord(character), len(bitmap), image.width))
        bitmap.extend(pack(image))
    assert len(bitmap) <= 65535
    lines = [f'/** @brief Packed {name} font pixels. */', f'static const uint8_t {name}_bitmap[] = {{']
    for start in range(0, len(bitmap), 16):
        lines.append('    ' + ', '.join(f'0x{value:02X}' for value in bitmap[start:start + 16]) + ',')
    lines.extend(['};', '', f'/** @brief {name.capitalize()} font glyph directory. */',
                  f'static const ssd1306_glyph_t {name}_glyphs[] = {{'])
    for codepoint, offset, width in glyphs:
        lines.append(f'    {{0x{codepoint:04X}, {offset}, {width}}},')
    lines.extend(['};', ''])
    return '\n'.join(lines)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('font', type=Path, help='Path to Lato-Semibold.ttf (OFL-1.1).')
    args = parser.parse_args()
    medium = ImageFont.truetype(str(args.font), 14)
    large = ImageFont.truetype(str(args.font), 30)
    printable = ''.join(chr(value) for value in range(32, 127)) + '°'
    assert set(SMALL) == set(printable)
    source = '''/**
 * @file ssd1306_fonts.c
 * @brief Generated monochrome fonts; regenerate with tools/generate_fonts.py.
 * @details Medium and large raster glyphs are derived from Lato Semibold.
 * Copyright (c) 2010-2011, Łukasz Dziedzic. Font data is under OFL-1.1;
 * see LICENSE.OFL. The small 5x7 glyphs are original to this component.
 */
#include "ssd1306_fonts_private.h"

#include <stddef.h>

// clang-format off

'''
    source += emit_font('small', printable, small_image)
    source += '\n' + emit_font('medium', printable, lambda ch: proportional_image(ch, medium, 16, 13))
    source += '\n' + emit_font('large', ' +,-.0123456789', lambda ch: proportional_image(ch, large, 28, 22))
    source += '''
/** @brief Supported font descriptors, indexed by ssd1306_font_t. */
static const ssd1306_font_data_t fonts[] = {
    {small_bitmap, small_glyphs, sizeof(small_glyphs) / sizeof(small_glyphs[0]), 7},
    {medium_bitmap, medium_glyphs, sizeof(medium_glyphs) / sizeof(medium_glyphs[0]), 16},
    {large_bitmap, large_glyphs, sizeof(large_glyphs) / sizeof(large_glyphs[0]), 28},
};

// clang-format on

const ssd1306_font_data_t *ssd1306_font_get(ssd1306_font_t font) {
    if ((uint32_t)font >= (uint32_t)(sizeof(fonts) / sizeof(fonts[0]))) {
        return NULL;
    }
    return &fonts[(uint32_t)font];
}
'''
    output = Path(__file__).resolve().parents[1] / 'ssd1306_fonts.c'
    output.write_text(source, encoding='utf-8')
    print(output)


if __name__ == '__main__':
    main()
