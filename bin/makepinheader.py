#!/usr/bin/env python3
import os, sys, getopt
import csv
import re
from os.path import basename

def writeHeader(headerFile, module):
	print("////////////////////////////////////////////////////////////////////////////////", file=headerFile)
	print("//", file=headerFile)
	print("// Auto-generated file, DO NOT EDIT!!!", file=headerFile)
	print("//", file=headerFile)
	print("// Command line:", file=headerFile)
	print("// ", *sys.argv, sep=' ', file=headerFile)
	print("//", file=headerFile)
	print("////////////////////////////////////////////////////////////////////////////////", file=headerFile)
	print("#ifndef ", module, file=headerFile)
	print("#define ", module, file=headerFile)	
	print("\n", file=headerFile)
	print("enum Pins {", file=headerFile)
	print("\tPA0 = 0, PA1, PA2, PA3, PA4, PA5, PA6, PA7, PA8, PA9, PA10, PA11, PA12, PA13, PA14, PA15,", file=headerFile)
	print("\tPB0, PB1, PB2, PB3, PB4, PB5, PB6, PB7, PB8, PB9, PB10, PB11, PB12, PB13, PB14, PB15,", file=headerFile)
	print("\tPC0, PC1, PC2, PC3, PC4, PC5, PC6, PC7, PC8, PC9, PC10, PC11, PC12, PC13, _PC14, _PC15,", file=headerFile)
	print("\tPD0, PD1, PD2, PD3, PD4, PD5, PD6, PD7, PD8, PD9, PD10, PD11, PD12, PD13, PD14, PD15,", file=headerFile)
	print("\tPE0, PE1, PE2, PE3, PE4, PE5, PE6, PE7, PE8, PE9, PE10, PE11, PE12, PE13, PE14, PE15,", file=headerFile)
	print("\tPF0, PF1, PF2, PF3, PF4, PF5, PF6, PF7, PF8, PF9, PF10, PF11, PF12, PF13, PF14, PF15,", file=headerFile)
	print("\tPG0, PG1, PG2, PG3, PG4, PG5, PG6, PG7, PG8, PG9, PG10, PG11, PG12, PG13, PG14, PG15,", file=headerFile)
	print("\tPH0, PH1, PH2, PH3, PH4, PH5, PH6, PH7, PH8, PH9, PH10, PH11, PH12, PH13, PH14, PH15,", file=headerFile)
	print("\tPI0, PI1, PI2, PI3, PI4, PI5, PI6, PI7, PI8, PI9, PI10, PI11, PI12, PI13, PI14, PI15,", file=headerFile)
	print("};\n", file=headerFile)
	print("\n#define PIN(a) (1 << ((a) % 16))", file=headerFile)
	print("extern GPIO_TypeDef* _gpioBanks[];", file=headerFile)
	print("#define BANK(a) (_gpioBanks[(a) / 16])\n", file=headerFile)


def writeFooter(headerFile, module):
	print("\n#endif // ", module, file=headerFile)

def toC(text):
	chars = "+-*/ @#$%^()."
	for c in chars:
		text = text.replace(c, "_")
	return text

def usage():
	print("Usage: ", basename(sys.argv[0]), "-p <pinfile.csv> -h <headerFile>", basename(sys.argv[0]))

def generateHeader(pinFile, headerFile):
	print("Generating header ", headerFile, " from ", pinFile)
	csvFile = csv.reader(open(pinFile))
	outFile = open(headerFile, 'w')
	module = toC(basename(headerFile)).upper()
	writeHeader(outFile, module)

	ignore = {"Signal", "RCC_OSC32_IN", "RCC_OSC32_OUT", "RCC_OSC_IN", "RCC_OSC_OUT"}
	userdefined = {"GPIO_Output", "GPIO_Input", "GPIO_Analog"}
	try:
		for row in csvFile:
			if len(row) > 4 and len(row[3]) > 0 and len(row[1]) > 0 and row[1] not in ignore and row[3] not in ignore:
				#name = row[4] if row[3] in userdefined else row[3]
				name = row[4] if row[4] else row[3]
				name = toC(name)
				pin = re.split(r'\W+', row[1])[0]
				print("#define\t", name, "\t", pin, "\t// Pin ", row[0], file=outFile); 
	except csv.Error as e:
		sys.exit('File {}, line {}: {}', format(pinFile, csvFile.line_num, e))
	writeFooter(outFile, module)

def main():
	if (len(sys.argv) < 2):
		usage()
		exit(2)
	try:
		opts, args = getopt.getopt(sys.argv[1:], "p:h:", ["pinfile=", "header="])
	except getopt.GetoptError as err:
		print(basename(sys.argv[0]) + ':', err)
		usage()
		sys.exit(2)

	for opt, arg in opts:
		if opt in ("-p", "--pinfile"):
			pinFile = arg
		elif opt in ("-h", "--headerfile"):
			headerFile = arg
		else:
			print("invalid argument '%s'" % (opt))

	try: pinFile
	except NameError: 
		print("No pin file specified!")
		usage()
		sys.exit(2)
	
	try: headerFile
	except NameError: 
		print("No header file specified!")
		usage()
		sys.exit(2)

	generateHeader(pinFile, headerFile)

if __name__ == "__main__":
	main()
