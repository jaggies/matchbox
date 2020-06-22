#!/usr/bin/env python3
import os, sys, getopt
from os.path import basename
from PIL import Image
from PIL import ImageFont, ImageDraw

class CharData:
	ch = 0
	pos = (0,0)
	size = (0,0)
	def __init__(self, ch, pos, size):
		self.ch = ch
		self.pos = pos
		self.size = size

def generateFont(cmd, imageWidth, imageHeight, fontPath, fontSize):
	fontName = basename(os.path.splitext(fontPath)[0]).replace("-", "_").lower()
	fontName += "_" + str(fontSize)
	foreground = 0 # (255, 255, 255)
	background = 1 # (0, 0, 0)

	#image = Image.new("RGB", (imageWidth,imageHeight), background)
	image = Image.new("1", (imageWidth,imageHeight), background)
	myfont = ImageFont.truetype(fontPath, fontSize)
	draw = ImageDraw.Draw(image)
	draw.fontmode = "1" # no anti-aliasing
	pos = (0,0)
	data = { }
	linemax = 0
	lastChar = 128 # chr(127)
	firstChar = 32 # chr(' ')
	for i in range(32,127):
		ch = chr(i)
		size = myfont.getsize(ch)
		linemax = max(linemax, size[1])
		if ((pos[0]+size[0]) > imageWidth):
			pos = (0, pos[1] + linemax)
			linemax = 0
		data[i] = CharData(chr(i), pos, size)
		draw.text(pos, ch, foreground, font=myfont)
		pos = (pos[0] + size[0], pos[1])

	arrayName = "%s_data" % (fontName)
	bitmapName = "im_bits"

	with open("%s.h" % (fontName), "w") as hsource:
		headerName = (fontName + "_h").upper()
		hsource.write("/*** Auto-generated. DO NOT EDIT! ***/\n")
		hsource.write("#ifndef %s\n#define %s\n" % (headerName, headerName))
		hsource.write("extern const Font %s;\n" % fontName)
		hsource.write("#endif // %s\n" % headerName)

	with open("%s.cpp" % (fontName), "w") as cppsource:
		cppsource.write("/*** Auto-generated file ('%s') . DO NOT EDIT! ***/\n" % ''.join(cmd))
		cppsource.write("#include <stdint.h>\n")
		cppsource.write("#include \"font.h\"\n")
		cppsource.write("#include \"%s.xbm\"\n" % (fontName))
		#cppsource.write("#include \"%s.h\"\n" % (fontName))
		cppsource.write("static const CharData %s[] = {\n" % (arrayName))
		for key in data:
			item = data[key]
			cppsource.write(" { %d /* %c */, %d, %d, %d, %d },\n" % (key, key, item.pos[0], item.pos[1], item.size[0], item.size[1]))
		cppsource.write("};\n")
		cppsource.write("extern const Font %s = {" % (fontName));
		cppsource.write("\t\"%s\", im_width, im_height, %d, &%s[0], (const uint8_t*) &%s[0] };\n" % (fontName, lastChar - firstChar, arrayName, bitmapName))
	image.save("%s.xbm"%(fontName))

def usage():
	print("Usage: ", basename(sys.argv[0]), "-f <ttf font path> -s <fontSize>> -w <bitmapWidth> -h <bitmapHeight>", basename(sys.argv[0]))

def main():
	imageWidth = 128
	imageHeight = 128
	fontPath = ""
	fontSize = 14
	if (len(sys.argv) < 2):
		usage()
		exit(2)
	try:
		opts, args = getopt.getopt(sys.argv[1:], "w:h:f:s:", ["imageWidth", "imageHeight", "font=", "size="])
	except getopt.GetoptError as err:
		print(basename(sys.argv[0]) + ':', err)
		usage()
		sys.exit(2)

	for opt, arg in opts:
		if opt in ("-f", "--font"):
			fontPath = arg
		elif opt in ("-s", "--size"):
			fontSize = int(arg)
		elif opt in ("-w", "--imageWidth"):
			imageWidth = int(arg)
		elif opt in ("-h", "--imageHeight"):
			imageHeight = int(arg)
		else:
			print("invalid argument '%s'" % (opt))
	generateFont(sys.argv, imageWidth, imageHeight, fontPath, fontSize)

if __name__ == "__main__":
	main()
