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


from inspect import trace
from typing import List, Dict, Any, Optional, Tuple, Callable
from pprint import pprint
import time
import logging
import sys
import traceback

logging.basicConfig()


class SJISUtil:
    """
    面、区、点

    第1面に限定してサポートする。第2面は第4水準文字が収録されているが、サポートしない。
    """

    @staticmethod
    def kuten_to_sjis(kuten:Tuple[int, int]) -> bytes:
        """
        区点コードをShiftJISのバイト列表現に変換する。第1面のみ対応。
        """
        ku = kuten[0]  # 1-94
        ten = kuten[1]  # 1-94
        b0:int = 0
        b1:int = 0

        if ku <= 62:
            # 第1区から第62区までの合計63区は0x81から0x9Fに、2区ずつ収容されている
            b0 = 0x81 + (ku - 1) // 2
        elif ku > 62:
            # 第63区以降は0xE0以降に、2区ずつ収容されている
            b0 = 0xE0 + (ku - 63)  // 2

        if ku & 0x01:
            # 奇数区の2バイト目は、0x40-0x7E, 0x80-0x9Eの範囲（0x7F=DELを除く。0x9Fは偶数区に割り当てられている）
            if ten <= 63:
                b1 = 0x40 + (ten - 1)
            else:
                b1 = 0x80 + (ten - 64)

        else:
            # 偶数区の2バイト目は、0x9F-0xFCの範囲
            b1 = 0x9F + (ten - 1)

        return bytes([b0, b1])


    @staticmethod
    def sjis_to_kuten(src:bytes) -> Tuple[int, int]:
        """
        ShiftJISのバイト列が表現する区点コードを返す。1バイト文字であった場合は、0区として返す（点番号はASCIIバイト表現そのまま）
        """
        if len(src) == 1:
            return bytes([0, src[0]])
        
        ku = src[0]
        if ku < 0xE0:
            ku = ku - 0x81
        else:
            ku = ku - 0xE0 + 62 // 2
        ku = ku * 2 + 1

        ten = src[1]
        if ten >= 0x9F:
            # 偶数区
            ku += 1
            ten = ten - 0x9F + 1
        else:
            # 奇数区
            if ten >= 0x80:
                ten = ten - 0x80 + 63 + 1
            else:
                ten = ten - 0x7E + 63

        return (ku, ten)


class SkkBinaryFile:
    def __init__(self) -> None:
        self.entries:List[Tuple[bytes, List[bytes]]] = []


    def add_entry(self, key:bytes, values:List[bytes]) -> None:
        self.entries.append((key, values))

    def sort(self) -> None:
        self.entries.sort(lambda v: v[0])

    @staticmethod
    def u8_to_bytes(u8value:int) -> bytes:
        assert(0 <= u8value <= 0xFF)
        return bytes([u8value])

    @staticmethod
    def u16_to_bytes(u16value:int) -> bytes:
        assert(0 <= u16value <= 0xFFFF)
        return bytes([ u16value & 0xFF, u16value >> 8])

    @staticmethod
    def u24_to_bytes(u24value:int) -> bytes:
        assert(0 <= u24value <= 0xFFFFFF)
        return bytes([u24value & 0xFF, (u24value >> 8) & 0xFF, u24value >> 16])

    @staticmethod
    def bytes_to_u8(b:bytes) -> int:
        return b[0]
    
    @staticmethod
    def bytes_to_u16(b:bytes) -> int:
        return (b[1] << 8) + b[0]

    @staticmethod
    def bytes_to_u24(b:bytes) -> int:
        return (b[2] << 16) + (b[1] << 8) + b[0]


    def build_globalheader(self, comment:bytes, maxlength:int) -> bytearray:
        data = bytearray()

        # Magic code 'TET' (Text encoding table)
        data.extend(b'TET')
        # File size (placeholder)
        data.extend(self.u24_to_bytes(0))
        # Comment length and Comment body
        comment = comment + b'\0'
        comment_len = len(comment)
        data.extend(self.u16_to_bytes(comment_len))
        data.extend(comment)
        # Max bytes of GB18030 bytes (Originaly Yomigana max byte length)
        data.extend(self.u16_to_bytes(maxlength))

        return data


    def build_index(self, indexingkeybytes:int = 1) -> bytearray:
        assert(indexingkeybytes == 1)

        indexentries:List[bytes] = []
        for entry in self.entries:
            firstbyte = entry[0][0:1]
            if not firstbyte in indexentries:
                indexentries.append(firstbyte)

        data = bytearray()

        # Magic code IDX'
        data.extend(b'IDX')
        # Length of this index
        data.extend(self.u24_to_bytes(0))

        # Index entry
        for ie in indexentries:
            # Index entry key length (Originaly Yomigana length)
            data.extend(self.u8_to_bytes(len(ie)))
            # Index entry key bytes
            data.extend(ie)
            # Associated address 24-bit (placeholder)
            data.extend(self.u24_to_bytes(0))

        # Index length field doesnt include 3-byte magic code and 3-byte index length field
        index_length = len(data) - 3 - 3
        # Replace index length field
        data[3:6] = self.u24_to_bytes(index_length)

        print("indexentry={}".format(repr(indexentries)))

        return data


    def build_entry(self) -> bytearray:
        data = bytearray()

        # Magic code
        data.extend(b'TBL')
        # Entry length (placeholder)
        data.extend(self.u24_to_bytes(0))

        for entry in self.entries:
            key_length = len(entry[0])
            # Key length (Originaly Yomigana length)
            data.extend(self.u8_to_bytes(key_length))
            # Key (Originaly Yomigana)
            data.extend(entry[0])
            # Number of candidates
            data.extend(self.u8_to_bytes(1))
            # Total byte of candidates
            totalbyte = (1 + len(entry[1][0]))
            data.extend(self.u16_to_bytes(totalbyte))
            # Byte of the candidate
            data.extend(self.u8_to_bytes(len(entry[1][0])))
            # Candidate
            data.extend(entry[1][0])

        # Exclude magic code (3-byte) and length field (3-byte)
        data[3:6] = self.u24_to_bytes(len(data) - 3 - 3)
        
        return data


    def search_address_with_cond(self, data:bytearray, cond:Callable[[bytes], bool]) -> Optional[int]:
        cur = 0

        tail = len(data)

        # Skip magic code
        cur += 3
        # Skip filesize
        cur += 3
        # Load comment length
        comment_length = self.bytes_to_u16(data[cur:cur+3])
        cur += 2
        # Skip comment body
        cur += comment_length
        # Skip max length field
        cur += 2

        # Skip magic code
        cur += 3
        # Load index length
        index_length = self.bytes_to_u24(data[cur:cur+3])
        cur += 3
        cur += index_length

        # Check TBL magic code
        try:
            assert(data[cur:cur+3] == b'TBL')
        except AssertionError:
            traceback.print_exc()
            print("Magic code assertion failed at {}; data={}".format(cur, repr(data[cur:cur+3])))
            print("comment_length={}, index_length={}".format(comment_length, index_length))
            print("data[cur-16:cur+16]={}".format(repr(data[cur-16:cur+16])))
            raise

        # Skip magic code (3-byte)
        cur += 3

        # Load and skip table length (3-byte)
        table_length = self.bytes_to_u24(data[cur:cur+3])
        cur += 3

        # print("comment_length={}, index_length={}, table_length={}".format(comment_length, index_length, table_length))

        while cur < tail:
            try:
                cur_addr = cur
                cur_yomigana_length = self.bytes_to_u8(data[cur:cur+1])
                cur += 1
                cur_yomigana = data[cur:cur+cur_yomigana_length]
                cur += cur_yomigana_length
                cur_cand_count = self.bytes_to_u8(data[cur:cur+1])
                cur += 1
                cur_cand_length = self.bytes_to_u16(data[cur:cur+2])
                cur += 2
                cur += cur_cand_length

                # print("cur=({}bytes)\"{}\", ({})({}bytes)".format(cur_yomigana_length, repr(cur_yomigana),
                #             cur_cand_count, cur_cand_length))

                if cond(cur_yomigana):
                    return cur_addr

            except IndexError:
                traceback.print_exc()
                return None


    def set_correct_index_address(self, data:bytearray) -> None:
        cur = 0

        # Skip magic code
        cur += 3
        # Skip filesize
        cur += 3
        # Load comment length
        comment_length = self.bytes_to_u16(data[cur:cur+3])
        cur += 2
        # Skip comment body
        cur += comment_length
        # Skip max length field
        cur += 2

        # Check index magic code
        try:
            assert(b'IDX' == data[cur:cur+3])
        except AssertionError:
            traceback.print_exc()
            print("Magic code assertion failed at {}; data={}".format(cur, data[cur:cur+3]))
            raise

        cur += 3
        # Load index length
        index_length = self.bytes_to_u24(data[cur:cur+3])
        cur += 3
        index_tail = cur + index_length

        while cur < index_tail:
            cur_ie_length = self.bytes_to_u8(data[cur:cur+1])
            cur += 1
            cur_ie_key = data[cur:cur+cur_ie_length]
            cur += cur_ie_length
            # Search address
            addr = self.search_address_with_cond(data, lambda v: v.startswith(cur_ie_key))
            if addr is not None:
                print("Address for index key {} is 0x{:x}({})".format(cur_ie_key, addr, addr))
                data[cur:cur+3] = self.u24_to_bytes(addr)
            else:
                logging.error("Address not found for key \"{}\"".format(cur_ie_key))
                data[cur:cur+3] = self.u24_to_bytes(0xFFFFFF)
            cur += 3


    def build_binary(self) -> bytes:
        data = bytearray()

        maxlen = max(len(v[1]) for v in self.entries)
        data.extend(self.build_globalheader(b"ShiftJIS to GB18030 convertion table.", maxlen))

        # Index
        data.extend(self.build_index(1))

        # Entry
        data.extend(self.build_entry())

        # Rewrite adresses in index
        self.set_correct_index_address(data)


        # Update File size
        filesize_idx = 3
        data[3:6] = self.u24_to_bytes(len(data))

        return data


    def save(self, filepath:str) -> None:
        data = self.build_binary()
        with open(filepath, "wb") as f:
            f.write(data)


def main():
    print("Start")

    # SJIS表記の全パターンのリストを作る
    srclist_sjis:List[bytes] = []

    for ku in range(1, 94+1):
        for ten in range(1, 94+1):
            sjisbytes = SJISUtil.kuten_to_sjis((ku, ten))
            srclist_sjis.append(sjisbytes)

    print("SJIS source list generated. {} items.".format(len(srclist_sjis)))

    # リストをGB18030へ変換する。エラーは除外する？
    convertedlist_gb18030: List[Tuple[bytes, bytes]] = []

    sjis_error_bytes:List[bytes] = []

    for i, src in enumerate(srclist_sjis):
        try:
            decoded_str = src.decode("shiftjis")

        except Exception:
            sjis_error_bytes.append(src)
            continue

        try:
            result = decoded_str.encode("gb18030")
            convertedlist_gb18030.append((src, result))

        except Exception as excep:
            traceback.print_exc()
            print("Decoding \"{}\"".format(decoded_str))
            raise

    print("Error count in decoding SJIS bytes: {}".format(len(sjis_error_bytes)))

    print("GB18030 convert table generated. {} items.".format(len(convertedlist_gb18030)))

    print("Differencial: {}".format(len(srclist_sjis) - len(sjis_error_bytes) - len(convertedlist_gb18030)))

    # SKKバイナリ辞書形式に押し込む

    skkbin = SkkBinaryFile()
    for entry in convertedlist_gb18030:
        skkbin.add_entry(entry[0], [ entry[1] ])
    
    filename = "CNVSJGB.TBL" + "-" + str(int(time.time())) + ".bin"
    print("Save to \"{}\"".format(filename))
    skkbin.save(filename)

    print("Finished.")


if __name__ == '__main__':
    main()
