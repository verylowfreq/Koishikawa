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


from typing import List, Optional, Tuple


romajitablefilepath = "romajitable.txt"

def c_binstr(data:bytes) -> str:
    s = ""
    for i, byte in enumerate(data):
        s += "\\x{:02x}".format(byte)
    s += ""
    return s

def format_romaji_hiragana(romaji:str, hiragana:str, hiraganabytelength:int) -> str:
    s = "{{ \"{}\", \"{}\", {} }}"
    return s.format(romaji, c_binstr(hiragana), hiraganabytelength)


def main():
    f = open(romajitablefilepath, "rb")
    s = ""

    s += """
typedef struct {
    const char* romaji;
    const char* hiragana;
    uint8_t hiraganabytelength;
} romajientry_t;

romajientry_t romajitable[] = {

""".lstrip()

    for line in f.readlines():
        line = line.strip(b'\n')
        if line[0:1] == b'#':
            continue
        print(line)
        romaji, hiragana, hiraganabytelen = line.split(b',')
        romaji = romaji.strip(b' ')
        hiragana = hiragana.strip(b' ')
        hiraganabytelen = hiraganabytelen.strip(b' ')
        s += "    "
        s += format_romaji_hiragana(romaji.decode("ascii"), hiragana, hiraganabytelen.decode("ascii"))
        s += ",\n"
    
    f.close()
    s = s.strip(",\n")
    s += "\n};\n"
    s += "\n#define ROMAJITABLE_ENTRIES (sizeof(romajitable) / sizeof(romajientry_t))\n"

    print(s)

    with open("romajidef.h", "w", encoding="ascii") as f2:
        f2.write(s)


if __name__ == '__main__':
    main()
