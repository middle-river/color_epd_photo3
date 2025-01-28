#!/usr/bin/python3

from PIL import Image
import sys

PALETTES = {
  'acep': {	# 7 colors.
    'paletteA': [ 0,  0,  0, 255, 255, 255,  0, 255,  0,  0,  0, 255, 255,  0,   0, 255, 255,   0, 255, 127,   0],	# Pure colors.
    'paletteB': [46, 67, 71, 176, 182, 178, 49,  99, 75, 54, 82, 105, 121, 64,  65, 151, 154,  77, 138, 102,  68],	# Actual colors.
    'paletteC': [35, 30, 53, 233, 235, 234, 53, 102, 78, 51, 53, 118, 203, 85,  82, 235, 215, 100, 207, 120,  95]},	# Sample colors.
  'spectra6': {	# 6 colors.
    'paletteA': [ 0,  0,  0, 255, 255, 255,  0, 255,  0,  0,  0, 255, 255,  0,   0, 255, 255,   0],	# Pure colors.
    'paletteB': [30, 27, 43, 156, 171, 169, 49,  92, 77, 35, 81, 136, 109, 26,  23, 153, 149,  23],	# Actual colors.
    'paletteC': [35, 30, 53, 233, 235, 234, 53, 102, 78, 51, 53, 118, 203, 85,  82, 235, 215, 100]}}	# Sample colors.

def main():
  assert len(sys.argv) == 5, 'usage: %s {acep|spectra6} {paletteA|paletteB|paletteC} <input file> <output file>'
  mode, palette, inpfile, outfile = sys.argv[1:]
  assert mode in {'acep', 'spectra6'}
  assert palette in {'paletteA', 'paletteB', 'paletteC'}

  pal = PALETTES[mode][palette]
  palimg = Image.new('P', (1, 1))
  palimg.putpalette(pal + pal[-3:] * (256 - len(pal) // 3))
  img = Image.open(inpfile).quantize(palette=palimg)  # Color reduction with Floyd-Steinberg dithering.
  pal = PALETTES[mode]['paletteA']  # Replace with paletteB for simulating the actual image.
  img.putpalette(pal + pal[-3:] * (256 - len(pal) // 3))
  img.save(outfile)

if __name__ == '__main__':
  main()
