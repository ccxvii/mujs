# Create utfdata.h from UnicodeData.txt and SpecialCasing.txt

import sys

tolower = []
toupper = []
tolower_full = []
toupper_full = []
isalpha = []

for line in open(sys.argv[1]).readlines():
	line = line.split(";")
	code = int(line[0],16)
	# if code > 65535: continue # skip non-BMP codepoints
	if line[2][0] == 'L':
		isalpha.append(code)
	if line[12]:
		toupper.append((code,int(line[12],16)))
	if line[13]:
		tolower.append((code,int(line[13],16)))

for line in open(sys.argv[2]).readlines():
	# SpecialCasing.txt -- code; lower; title; upper; (condition;)? # comment
	line = line.strip()
	if len(line) == 0:
		continue
	if line[0] == "#":
		continue
	line = line.split(";")
	code = int(line[0],16)
	lower = line[1].strip()
	upper = line[3].strip()
	if len(lower) == 0 or len(upper) == 0:
		continue
	condition = line[4].split("#")[0].strip()
	if len(condition) > 0:
		continue
	lower = list(map(lambda x: int(x,16), lower.split(" ")))
	upper = list(map(lambda x: int(x,16), upper.split(" ")))
	if lower[0] != code:
		tolower_full.append([code] + lower)
	if upper[0] != code:
		toupper_full.append([code] + upper)

tolower_full.sort()
toupper_full.sort()

def dumpalpha():
	table = []
	prev = 0
	start = 0
	for code in isalpha:
		if code != prev+1:
			if start:
				table.append((start,prev))
			start = code
		prev = code
	table.append((start,prev))

	print("")
	print("static const Rune ucd_alpha2[] = {")
	for a, b in table:
		if b - a > 0:
			print(hex(a)+","+hex(b)+",")
	print("};");

	print("")
	print("static const Rune ucd_alpha1[] = {")
	for a, b in table:
		if b - a == 0:
			print(hex(a)+",")
	print("};");

def dumpmap(name, input):
	table = []
	prev_a = 0
	prev_b = 0
	start_a = 0
	start_b = 0
	for a, b in input:
		if a != prev_a+1 or b != prev_b+1:
			if start_a:
				table.append((start_a,prev_a,start_b))
			start_a = a
			start_b = b
		prev_a = a
		prev_b = b
	table.append((start_a,prev_a,start_b))

	print("")
	print("static const Rune " + name + "2[] = {")
	for a, b, n in table:
		if b - a > 0:
			print(hex(a)+","+hex(b)+","+str(n-a)+",")
	print("};");

	print("")
	print("static const Rune " + name + "1[] = {")
	for a, b, n in table:
		if b - a == 0:
			print(hex(a)+","+str(n-a)+",")
	print("};");

def dumpmultimap(name, table, w):
	print("")
	print("static const Rune " + name + "[] = {")
	for list in table:
		list += [0] * (w - len(list))
		print(",".join(map(hex, list)) + ",")
	print("};")

print("/* This file was automatically created from " + sys.argv[1] + " */")
dumpalpha()
dumpmap("ucd_tolower", tolower)
dumpmap("ucd_toupper", toupper)
dumpmultimap("ucd_tolower_full", tolower_full, 4)
dumpmultimap("ucd_toupper_full", toupper_full, 5)
