# This file is part of eVic SDK.
#
# eVic SDK is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# eVic SDK is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with eVic SDK.  If not, see <http://www.gnu.org/licenses/>.
#
# Copyright (C) 2016 ReservedField

from __future__ import division
from PIL import Image
from PIL import ImageOps
from bitarray import bitarray

class Bitmap(object):
	def __init__(self, img, imgBits):
		self.img = img
		self.width, self.height = img.size
		self.imgBits = imgBits

	@classmethod
	def fromImage(cls, img):
		if not isinstance(img, Image.Image):
			raise TypeError('PIL image expected')

		# Convert to monochrome if needed
		# We go through grayscale for better results
		img = img.copy()
		if img.mode != '1':
			if img.mode != 'L':
				img = img.convert('L')
			img = img.point(lambda x: 0 if x < 32 else 255, '1')
		width, height = img.size

		# Convert to column-major, 8 pixels/byte, topmost = LSB
		imgPixels = img.load()
		imgBits = bitarray(endian='little')
		for x in range(width):
			for y in range(height):
				imgBits.append(imgPixels[x, y])
			# Pad column to byte boundary
			imgBits.fill()

		return cls(img, imgBits)

	@classmethod
	def fromBytes(cls, imgBytes, width, height):
		if not isinstance(imgBytes, bytes):
			raise TypeError('bytes expected')

		padHeight = (height + 7) & ~7
		numBytes = width * padHeight // 8
		if len(imgBytes) < numBytes:
			raise ValueError('raw input image too short')

		# Decode from column-major, 8 pixels/byte, topmost = LSB
		imgBits = bitarray(endian='little')
		imgBits.frombytes(imgBytes)
		img = Image.new('1', (width, height))
		imgPixels = img.load()
		for x in range(width):
			for y in range(height):
				imgPixels[x, y] = 255 if imgBits[x*padHeight + y] else 0

		return cls(img, imgBits)

	def invert(self):
		self.img = self.img.point(lambda x: 255 - x)
		self.imgBits.invert()

	def toAsciiArtStr(self, fromBits):
		strList = []

		# Generate either from bit array or PIL image to check conversion
		if fromBits:
			padHeight = (self.height + 7) & ~7
			for y in range(self.height):
				for x in range(self.width):
					strList.append('#' if self.imgBits[x * padHeight + y] else ' ')
				strList.append('\n')
		else:
			imgPixels = self.img.load()
			for y in range(self.height):
				for x in range(self.width):
					strList.append('#' if imgPixels[x, y] else ' ')
				strList.append('\n')

		return ''.join(strList)

	def toCArrayStr(self, prefix, lineElms):
		strList = []
		imgBytes = self.imgBits.tobytes() if hasattr(self.imgBits, 'tobytes') else imgBits.tostring()
		byteLines = [imgBytes[i:i+lineElms] for i in range(0, len(imgBytes), lineElms)]
		for idx, line in enumerate(byteLines):
			strList.append(prefix + ', '.join(['0x{:02X}'.format(b) for b in bytearray(line)]))
			if idx != len(byteLines) - 1:
				strList.append(',\n')
		return ''.join(strList)
