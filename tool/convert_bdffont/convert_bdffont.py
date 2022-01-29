# Copyright 2022 verylowfreq ( https://github.com/verylowfreq/ )
#
# Permission is hereby granted, free of charge, to any person obtaining a copy 
# of this software and associated documentation files (the "Software"), to 
# deal in the Software without restriction, including without limitation the 
# rights to use, copy, modify, merge, publish, distribute, sublicense, and/or 
# sell copies of the Software, and to permit persons to whom the Software is 
# furnished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included in 
# all copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR 
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, 
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE 
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER 
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING 
# FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER 
# DEALINGS IN THE SOFTWARE.


from os import altsep
from typing import Iterable, Tuple, List, Optional, NoReturn, Dict, Iterable
import dataclasses
from dataclasses import dataclass
import logging
import traceback
import sys
from pprint import pprint, pformat


logger = logging.getLogger(__name__)


# Tofu glyph for padding
tofuglyph_7 = bytes([
    0xFF, 0x0F, 0x0F, 0x01, 0x01, 0x01, 0xFF,
    0x3F, 0x20, 0x20, 0x20, 0x20, 0x3F, 0x3F
])
tofuglyph_14 = bytes([
    0xFF, 0x0F, 0x0F, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0xFF,
    0x3F, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x3F, 0x3F
])

@dataclass
class Glyph:
    serialnumber:int = 0
    glyphencoding:int = 0
    kuten:Tuple[int, int] = (0, 0)
    size:Tuple[int, int] = (0, 0)
    bitmap:List[int] = dataclasses.field(default_factory=list)
    serialized_bitmap_bytes:Optional[bytes] = None


    def dump(self) -> str:
        s = "<Font {}, KuTen={},{}, size={}x{}\n".format(self.serialnumber, self.kuten[0], self.kuten[1], self.size[0], self.size[1])
        s += "=" * self.size[0] +   "\n"
        # for row in self.bitmap:
        #     s += "{:02x} ".format(row)
        # s += "\n"
        # s += "=" * self.size[0] + "\n"
        # for row in self.bitmap:
        #     s += "  "
        #     for col in range(self.size[0]):
        #         idx = self.size[0] - col - 1
        #         bit = ((row >> idx) & 1) != 0
        #         s += "*" if bit else " "
        #     s += "\n"
        # s += "=" * self.size[0] + "\n"
        # s += ">"

        for y in range(0, self.size[1]):
            for x in range(0, self.size[0]):
                dot = self.get_dot(x, y)
                if dot:
                    s += "*"
                else:
                    s += " "
            s += "\n"
        s += "\n"
        s += "=" * self.size[0] + "\n"
        s += ">"

        return s


    def serialize_bitmap(self) -> bytes:
        """
        グリフのビットマップをbytesにする。
        """
        # 1行ごとにドットを処理する。
        verticalbytelength = ((self.size[1] + 7) // 8)
        bitmapbytelength = self.size[0] * verticalbytelength
        bitmap = bytearray(bitmapbytelength)
        for verticalbytecnt in range(verticalbytelength):
            ybase = verticalbytecnt * 8
            for x in range(self.size[0]):
                for i in range(8):
                    y = ybase + i
                    if y >= self.size[1]:
                        pass
                    else:
                        dot = self.get_dot(x, y)
                        if dot:
                            byteindex = self.size[0] * verticalbytecnt + x
                            bitmap[byteindex] |= 0x01 << i

        self.serialized_bitmap_bytes = bitmap.copy()

        return bitmap


    def get_dot(self, x:int, y:int) -> bool:
        try:
            listindex = y
            bitshift = (self.size[0] - x)
            dot = self.bitmap[listindex] & (0x01 << bitshift)
            return True if dot else False
        except:
            s = "<Font {}, KuTen={},{}, size={}x{}\n".format(self.serialnumber, self.kuten[0], self.kuten[1], self.size[0], self.size[1])
            print("ERROR: get_dot() : x={}, y={}, listindex={}, bitshift={}".format(x,y,listindex,bitshift))
            print(s)
            # raise
            return False

    def dump_serialized_bitmap(self) -> None:

        #FIXME: いちおう正しいが、裏返ったり回転しているので、直せるとベター

        # 14ドットフォントでは、1列は2バイトで構成されている
        verticalbytecount = (self.size[1] + 7) // 8
        # 14ドットフォントは、28バイト
        bitmapbytelength = self.size[0] * verticalbytecount
        print(len(self.serialized_bitmap_bytes),bitmapbytelength)
        assert(len(self.serialized_bitmap_bytes) == bitmapbytelength)
        # 1列ずつ
        for x in range(self.size[0]):
            # 1行ずつ
            for y in range(self.size[1]):
                byteoffset = (y // 8) * self.size[0] + x
                bitshift = y % 8
                # print("x={}, y={}, byteoffset={}, bitshift={}".format(x,y,byteoffset,bitshift))
                dot = self.serialized_bitmap_bytes[byteoffset] & (0x01 << bitshift)
                print("*" if dot else " ", end="")

            print("")

        # # 1列ずつ
        # for x in range(self.size[0]):
        #     for horizontalbytecnt in range(0, verticalbytecount):
        #         byteoffsetbase = verticalbytecount * horizontalbytecnt
        #         byteoffset = byteoffsetbase + x
        #         for bit in range(0, 8):
        #             bitshift = 7 - bit
        #             dot = self.serialized_bitmap_bytes[byteoffset] & (0x01 << bitshift)
        #             if dot:
        #                 print("*", end="")
        #             else:
        #                 print(" ", end="")
        #     print("")


class FontSerializer:

    GLOBAL_HEADER_BYTELENGTH:int = 2 + 3
    INDEX_HEADER_BYTELENGTH:int = 2 + 3
    GLYPH_DEFINITIONS_HEADER_BYTELENGTH:int = 2 + 3

    GLYPH_BYTELENGTH_7:int = 1 * 14
    GLYPH_BYTELENGTH_14:int = 2 * 14

    def __init__(self) -> None:
        self.glyphs:List[Glyph] = []


    def build_index(self) -> bytes:
        buf = bytearray()
        indexing_ku = []
        # for ku in range(1,94+1):
        for ku in range(0, 0xFF+1):
            current_glyphs = [ g for g in self.glyphs if g.kuten[0] == ku]
            if len(current_glyphs) > 0:
                indexing_ku.append(ku)
        # (2; Magic number) + (3; index length) + (n; ku table)
        indexlength = len(indexing_ku) * 4
        buf.extend(b'TB')
        buf.extend([indexlength & 0xFF, (indexlength >> 8) & 0xFF, (indexlength >> 16) & 0xFF])
        indexing_ku.sort()
        for i, ku in enumerate(indexing_ku):
            # (28; bytes per a glyph) * (94; glyphs in a Ku)
            bytes_glyphtable_per_ku = self.GLYPH_BYTELENGTH_14 * 94
            bytes_glyphtable_pre_ku_zero = self.GLYPH_BYTELENGTH_7 * 256
            if ku == 0:
                offset = self.GLOBAL_HEADER_BYTELENGTH + indexlength + self.GLYPH_DEFINITIONS_HEADER_BYTELENGTH
            else:
                offset = self.GLOBAL_HEADER_BYTELENGTH + indexlength + self.GLYPH_DEFINITIONS_HEADER_BYTELENGTH + bytes_glyphtable_pre_ku_zero + bytes_glyphtable_per_ku * (i - 1)

            buf.extend([ku, offset & 0xFF, (offset >> 8) & 0xFF, (offset >> 16) & 0xFF])

            print("DEBUG: Ku {} offset={}".format(ku, offset))

        return buf


    def build_glyphs(self) -> bytes:
        self.glyphs.sort(key=lambda g: g.kuten[0] * 0x100 + g.kuten[1])
        buf = bytearray()

        for glyph in self.glyphs:
            if not glyph.serialized_bitmap_bytes or len(glyph.serialized_bitmap_bytes) < 1:
                glyph.serialize_bitmap()
            if len(glyph.serialized_bitmap_bytes) < 1:
                print("Glyph serialized_bitmap_bytes not generated correctly.: ", glyph)
                assert(len(glyph.serialized_bitmap_bytes) > 1)
            buf.extend(glyph.serialized_bitmap_bytes)
        
        return buf


    def serialize(self) -> bytes:
        # FIXME: Only 14dot height font supported.
        assert(self.glyphs[0].size[1] == 14)

        ba = bytearray()

        bytes_index = self.build_index()
        bytes_glyphs = self.build_glyphs()
        # (2; 'FN') + (3; Total length)
        totallength = len(bytes_index) + len(bytes_glyphs)

        ba.extend(b'FN')
        ba.extend([totallength & 0xFF, (totallength >> 8) & 0xFF, (totallength >> 16) & 0xFF])
        ba.extend(bytes_index)
        ba.extend(bytes_glyphs)

        return ba


class BDFParser:
    def __init__(self) -> None:
        self.fontfilepath:Optional[str] = None
        self.fontglyphs:List[Glyph] = []


    def load(self, path:str) -> None:
        self.fontfilepath = path


    def get_entry(self, key:str) -> Optional[str]:
        with open(self.fontfilepath) as f:
            f.seek(0)
            while True:
                line = f.readline()

                if line == '':
                    # Reached the end of file
                    logger.debug("Reached EOF")
                    break

                elif line.startswith('STARTCHAR'):
                    # Estimate to be reached the end of header
                    logger.debug("Estimate to be reached the end of header.")
                    break
                
                elif line.startswith('COMMENT'):
                    # Just ignore comment line
                    continue

                else:
                    line = line.strip()

                    try:
                        if line.index(' '):
                            k, v = line.split(' ', maxsplit=1)
                        elif line.index('\t'):
                            k, v = line.index('\t', maxsplit=1)
                    except ValueError:
                        # print("Separator not found.")
                        k = line
                        v = ""
                    if k == key:
                        return v


    def parse_all(self) -> int:
        with open(self.fontfilepath) as f:
            try:
                is_in_bitmap_section:bool = False
                linenum = 1
                while True:
                    line = f.readline()
                    if line == '':
                        raise EOFError()

                    if line.startswith("STARTCHAR"):
                        cur = Glyph()
                        line = line.replace("STARTCHAR", "").strip()
                        cur.serialnumber = int(line, base=16)

                    elif line.startswith("ENCODING"):
                        line = line.replace("ENCODING", "").strip()
                        # cur.encbytes = self.hexstr_to_bytes(line)
                        cur.glyphencoding = int(line, base=10)
                        jiscode:bytes = self.decimalstr_to_le2bytes(line)
                        cur.kuten = self.jiscode_to_kuten(jiscode)

                    elif line.startswith("BBX"):
                        line = line.replace("BBX", "").strip()
                        elems = line.split(' ')
                        cur.size = (int(elems[0]), int(elems[1]))

                    elif line.startswith("BITMAP"):
                        is_in_bitmap_section = True

                    elif line.startswith("ENDCHAR"):
                        self.fontglyphs.append(cur)
                        cur = None
                        is_in_bitmap_section:bool = False

                    elif is_in_bitmap_section:
                        val = self.hexstr_to_int(line)
                        cur.bitmap.append(val)

                    linenum += 1

            except EOFError:
                logger.info("Reached end of font file. (linenum {})".format(linenum - 1))
                return len(self.fontglyphs)


            except:
                logger.error("Exception at linenum {}".format(linenum))
                logger.error(traceback.format_exc())
                raise


    # @staticmethod
    # def int_to_bytes(val:int) -> bytes:
    #     if val < 0xFF:
    #         return bytes([val])
    #     elif val < 0xFFFF:
    #         return bytes([val & 0xFF, val >> 8])
    #     else:
    #         raise NotImplementedError()


    @staticmethod
    def hexstr_to_int(hexstr:str) -> int:
        val = 0
        len_in_byte = len(hexstr) // 2
        for i in range(len_in_byte):
            b = int(hexstr[i*2:i*2+2], base=16)
            val += b * int(pow(0x100, len_in_byte - i - 1))
        return val

    @staticmethod
    def hexstr_to_bytes(hexstr:str) -> bytes:
        ba = bytearray()
        for i in range(len(hexstr) // 2):
            b = int(hexstr[i*2:i*2+1], base=16)
            ba.append(b)
        return ba

    @staticmethod
    def decimalstr_to_le2bytes(decstr:str) -> bytes:
        ba = bytearray(2)
        val = int(decstr, 10)
        ba[0] = (val >> 8) & 0xFF
        ba[1] = val & 0xFF
        return ba

    @staticmethod
    def jiscode_to_kuten(jiscode:bytes) -> Tuple[int, int]:
        if jiscode[0] == 0:
            return (0, jiscode[0])
        else:
            return (jiscode[0] - 0x20, jiscode[1] - 0x20)


def main() -> NoReturn:
    logging.basicConfig(stream=sys.stdout, level=logging.DEBUG)

    glyphs:List[Glyph] = []


    parser = BDFParser()
    parser.load("shnmk14.bdf")
    parsedglyphs = parser.parse_all()

    # pprint(parser.fontglyphs[0])
    # print(parser.fontglyphs[0].dump())
    # parser.fontglyphs[0].serialize_bitmap()
    # print(parser.fontglyphs[0].dump_serialized_bitmap())
    # pprint(parser.fontglyphs[100]  )
    # print(parser.fontglyphs[100].dump())
    # parser.fontglyphs[100].serialize_bitmap()
    # print(parser.fontglyphs[100].dump_serialized_bitmap())
    # pprint(parser.fontglyphs[1000]  )
    # print(parser.fontglyphs[1000].dump())
    # parser.fontglyphs[1000].serialize_bitmap()
    # print(parser.fontglyphs[1000].dump_serialized_bitmap())
    # print("Parsed {} glyphs.".format(parsedglyphs))


    print("Parsed {} glyphs.".format(parsedglyphs))

    glyphs.extend(parser.fontglyphs)

    parser2 = BDFParser()
    parser2.load("shnm7x14a.bdf")
    parser2_glyphcount = parser2.parse_all()
    print("From shnm7x14a.bdf, {} glyphs loaded.".format(parser2_glyphcount))
    for glyph in parser2.fontglyphs:
        ku = 0
        ten = glyph.glyphencoding
        glyph.kuten = (ku, ten)
    # pprint(parser2.fontglyphs)

    glyphs.extend(parser2.fontglyphs)

    tofuglyphobj = Glyph()
    tofuglyphobj.kuten = (0xFF, 0)
    tofuglyphobj.size = (14, 14)
    tofuglyphobj.serialized_bitmap_bytes = tofuglyph_14
    tofuglyphobj.dump_serialized_bitmap()

    glyphs.extend([tofuglyphobj])


    for ku in range(0, 0xFF+1):
        glyphs_in_ku = [ g for g in glyphs if g.kuten[0] == ku ]
        glyphs_in_ku.sort(key=lambda e: e.kuten[1])
        # if len(glyphs) > 0:
        #     # print("Ku {}: first glyph".format(ku))
        #     # pprint(glyphs[0])
        #     glyphs[0].dump()
        #     glyphs[0].serialize_bitmap()
        #     glyphs[0].dump_serialized_bitmap()
        #     pass
        if len(glyphs_in_ku) > 0:
            print("Ku {} has {} glyphs.".format(ku, len(glyphs_in_ku)))


    append_tofu_glyphs:List[Glyph] = []

    # Ku 0 is special space for ASCII code stores 256 glyphs.
    for ku in range(0, 1):
        asciiglyphs = [ g for g in glyphs if g.kuten[0] == 0 ]
        for ten in range(0, 0xFF+1):
            glyphlist = [ g for g in asciiglyphs if g.kuten[1] == ten ]
            if len(glyphlist) < 1:
                # Add tofu
                newglyph = Glyph()
                newglyph.kuten = (ku, ten)
                newglyph.size = (7, 14)
                newglyph.serialized_bitmap_bytes = tofuglyph_7
                append_tofu_glyphs.append(newglyph)
            elif len(glyphlist) == 1:
                # OK
                continue
            else:
                # Bug
                print("ERROR: ku={},ten={}, len(glyphlist)={}, glyphlist={}".format(ku, ten, len(glyphlist), glyphlist))
                assert(len(glyphlist) == 1)


    for ku in range(1, 94):
        glyphs_in_ku = [ g for g in glyphs if g.kuten[0] == ku ]
        if len(glyphs_in_ku) < 1:
            continue
        for ten in range(1, 94+1):
            glyph = ([ g for g in glyphs_in_ku if g.kuten[1] == ten])
            if len(glyph) < 1:
                # Add tofu
                newglyph = Glyph()
                newglyph.kuten = (ku, ten)
                if ku == 0:
                    newglyph.size = (7, 14)
                    newglyph.serialized_bitmap_bytes = tofuglyph_7
                else:
                    newglyph.size = (14, 14)
                    newglyph.serialized_bitmap_bytes = tofuglyph_14
                append_tofu_glyphs.append(newglyph)
            elif len(glyph) == 1:
                # OK
                continue
            else:
                # Bug
                assert(len(glyph) == 1)

    # Ku 0xFF is special space for storing Zenkaku Tofu.
    for ku in range(0xFF, 0xFF+1):
        pass

    glyphs.extend(append_tofu_glyphs)

    glyphs.sort(key=lambda e: e.kuten[0] * 0x100 + e.kuten[1])

    print("=" * 16)

    for ku in range(0, 0xFF+1):
        glyphs_in_ku = [ g for g in glyphs if g.kuten[0] == ku ]
        glyphs_in_ku.sort(key=lambda e: e.kuten[1])
        # if len(glyphs) > 0:
        #     # print("Ku {}: first glyph".format(ku))
        #     # pprint(glyphs[0])
        #     glyphs[0].dump()
        #     glyphs[0].serialize_bitmap()
        #     glyphs[0].dump_serialized_bitmap()
        #     pass
        if len(glyphs_in_ku) > 0:
            print("Ku {} has {} glyphs.".format(ku, len(glyphs_in_ku)))

    print("=" * 16)

    print("Total glyphs: {} glyphs.".format(len(glyphs)))

    # for asciiglyph in [ g for g in glyphs if g.kuten[0] == 0 ]:
    #     print(asciiglyph)

    if True:
        print("Serialize...")
        serializer = FontSerializer()
        serializer.glyphs = glyphs
        fontbin = serializer.serialize()


    if True:
        print("Write out to font14jis.bin...")
        with open("fnt14jis.bin", "wb") as f:
            f.write(fontbin)


    def print_glyph(glyphs:List[Glyph], ku:int, ten:int):
        candidates = [ g for g in glyphs if g.kuten == (ku, ten) ]
        if len(candidates) == 1:
            print("Ku={},Ten={} found.".format(ku, ten))
            print(candidates[0].dump())

    # print_glyph(glyphs, 0, 0)
    # print_glyph(glyphs, 0, 1)
    for i in range(0, 0x30+1):
        print_glyph(glyphs, 0, i)
    # print_glyph(glyphs, 0, 0x21)
    # print_glyph(glyphs, 0, 0x31)
    # print_glyph(glyphs, 0, 0x41)

    # print_glyph(glyphs, 5, 2)
    # print_glyph(glyphs, 3, 33)
    # print_glyph(glyphs, 16, 2)
    # print_glyph(glyphs, 27, 1)
    # print_glyph(glyphs, 39, 1)

    print("Exit.")


if __name__ == '__main__':
    main()
