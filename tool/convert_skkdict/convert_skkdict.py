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


import dataclasses
import sys
import copy
import time
from typing import Callable, Dict, List, Tuple, Optional
import traceback
import logging
from pprint import pprint


"""
テキスト形式のSKK辞書を、独自のバイナリ形式に変換するスクリプト
"""


def init_logger() -> logging.Logger:
    logger = logging.getLogger(__name__)
    logger_handler = logging.StreamHandler()
    logger_handler.setLevel(logging.DEBUG)
    logger_handler.setFormatter(logging.Formatter(fmt="%(asctime)s [%(levelname)s] %(message)s"))
    logger.addHandler(logger_handler)
    logger.setLevel(logging.DEBUG)
    return logger

logger = init_logger()


ENC_SHIFTJIS:str = "shiftjis"


class Util:
    @staticmethod
    def convert_2bytes_to_lebytes(val:int) -> bytes:
        if not (0 <= val <= 0xFFFF):
            raise ValueError("Out of range for uint16")
        return bytes([ val & 0xFF, (val >> 8) & 0xFF])

    @staticmethod
    def convert_3bytes_to_lebytes(val:int) -> bytes:
        if not (0 <= val <= 0xFFFFFF):
            raise ValueError("Out of range for uint24")
        return bytes([ val & 0xFF, (val >> 8) & 0xFF, (val >> 16) & 0xFF])

    @staticmethod
    def convert_4bytes_to_lebytes(val:int) -> bytes:
        if not (0 <= val <= 0xFFFFFFFF):
            raise ValueError("Out of range for uint32")
        return bytes([ val & 0xFF, (val >> 8) & 0xFF, (val >> 16) & 0xFF, (val >> 24) & 0xFF])

    @staticmethod
    def convert_lebytes_to_uint8(b:bytes) -> int:
        if isinstance(b, int):
            return b
        else:
            assert(len(b) == 1)
            return b[0]

    @staticmethod
    def convert_lebytes_to_uint16(b:bytes) -> int:
        if len(b) == 1:
            return b[0]
        elif len(b) == 2:
            return b[1] * 0x100 + b[0]
        else:
            logger.error("Util.convert_lebytes_to_uint16(): b={}".format(b))
            raise ValueError("Not 1byte or 2bytes value")


    @staticmethod
    def convert_lebytes_to_uint24(b:bytes) -> int:
        if len(b) == 3:
            return b[2] * 0x10000 + b[1] * 0x100 + b[0]
        else:
            logger.error("Util.convert_lebytes_to_uint24(): b={}".format(b))
            raise ValueError("Not 3bytes value")

    @staticmethod
    def convert_uint8_to_bytes(u8value:int) -> bytes:
        assert(0 <= u8value <= 0xFF)
        return bytes([ u8value ])

    @staticmethod
    def convert_uint16_to_bytes(u16value:int) -> bytes:
        assert(0 <= u16value <= 0xFFFF)
        return bytes([
            u16value & 0xFF,
            (u16value >> 8) & 0xFF
        ])

    @staticmethod
    def convert_uint24_to_bytes(u24value:int) -> bytes:
        assert(0 <= u24value <= 0xFFFFFF)
        return bytes([
            u24value & 0xFF,
            (u24value >> 8) & 0xFF,
            (u24value >> 16) & 0xFF
        ])

    @staticmethod
    def convert_uint32_to_bytes(u32value:int) -> bytes:
        assert(0 <= u32value <= 0xFFFFFFFF)
        return bytes([
            u32value & 0xFF,
            (u32value >> 8) & 0xFF,
            (u32value >> 16) & 0xFF,
            (u32value >> 24) & 0xFF
        ])


@dataclasses.dataclass
class IndexEntry_str:
    key:str = ""
    value:int = 0

    def to_bytes(self) -> bytes:
        b = bytearray()
        keybytes = self.key.encode(ENC_SHIFTJIS)
        b.extend(Util.convert_uint8_to_bytes(len(keybytes)))
        b.extend(keybytes)
        b.extend(Util.convert_uint24_to_bytes(self.value))
        return b


@dataclasses.dataclass
class DataEntry_str:
    key:str = ""
    values:List[str] = dataclasses.field(default_factory=list)

    def to_bytes(self) -> bytes:
        """
        オブジェクトをもとに、バイナリ形式を生成する
        """
        b = bytearray()

        yomiganabytes = self.key.encode("shiftjis")

        # キーのバイト数
        b.extend([ len(yomiganabytes) & 0x7F ])

        # キーのバイト列
        b.extend(yomiganabytes)

        # データの数
        b.extend([ len(self.values) ])
        
        # データ群の領域の合計バイト数 = 個数 * ( (uint8:変換候補の長さ) + 変換候補のバイト列 )
        candidatesbytelength = (len(self.values) * 1) + sum([ len(v.encode("shiftjis")) for v in self.values ])
        b.extend(Util.convert_2bytes_to_lebytes(candidatesbytelength))

        # データそれぞれを追加する
        for candidate in self.values:
            candidatebytes = candidate.encode("shiftjis")
            # この候補のバイト数
            b.extend([ len(candidatebytes) ])
            # この候補のバイト列
            b.extend(candidatebytes)

        return bytes(b)

    
    def to_json(self) -> str:
        return '{{ "key": "{}", "values": {} }}'.format(self.key, repr(self.values))


class SkkDictBinaryConverter:

    SJIS_HIRAGANA_LIST = "ぁ あ ぃ い ぅ う ぇ え ぉ お か が き ぎ く ぐ け げ こ ご さ ざ し じ す ず せ ぜ そ ぞ た だ ち ぢ っ つ づ て で と ど な に ぬ ね の は ば ぱ ひ び ぴ ふ ぶ ぷ へ べ ぺ ほ ぼ ぽ ま み む め も ゃ や ゅ ゆ ょ よ ら り る れ ろ ゎ わ ゐ ゑ を ん".split(" ")


    def __init__(self) -> None:
        self.entries:List[DataEntry_str] = []
        self.indexentries:List[IndexEntry_str] = []
        self.maximum_index_key_length = -1
        self.comment_str = ""


    def load(self, filepath:str) -> None:
        with open(filepath, "r", encoding="shiftjis") as f:
            self.original_content:List[str] = f.readlines()

    def set_comment(self, comment:str) -> None:
        self.comment_str = comment


    def read_entry(self, line:str) -> DataEntry_str:
        """
        変換エントリの行をパースし、オブジェクトとして返す。
        """

        try:
            yomigana, candidates = line.split(' ', 1)
            candidates = [ v for v in candidates.split('/') if len(v) > 0 ]
            # 注記を取り除く（角変換候補のうち ';' 以降）
            candidates = [ v.split(';')[0] for v in candidates ]
        except:
            print("Exception in \"{}\"".format(line))
            traceback.print_exc()
            raise

        return DataEntry_str(yomigana, candidates)


    def load_entries(self) -> None:
        self.yomigana_maxlen = -1

        # テキスト形式の辞書ファイルを1行ずつパースする
        for line in self.original_content:
            line = line.strip()
            if len(line) < 1:
                continue
            if line.startswith(';'):
                continue
            ent = self.read_entry(line)

            if self.is_entry_to_be_removed(ent):
                pass
            else:
                self.entries.append(ent)
                yomiganalen = len(ent.key.encode(ENC_SHIFTJIS))
                if yomiganalen > self.yomigana_maxlen:
                    self.yomigana_maxlen = yomiganalen

        # self.entries.sort(key=lambda v: Util.convert_lebytes_to_uint16(v[0][0].encode("shiftjis")))
        # self.entries.sort(key=lambda v: v[0])
        # インデックス構築の都合で、順序を逆転する
        # self.entries.reverse()
        # self.entries.sort(key=lambda v: v[0].encode("shiftjis"), reverse=True)

        # あ → ん の方向で、各ひらがな内は逆順で並べる
        
        # hiraganalist = "ぁ あ ぃ い ぅ う ぇ え ぉ お か が き ぎ く ぐ け げ こ ご さ ざ し じ す ず せ ぜ そ ぞ た だ ち ぢ っ つ づ て で と ど な に ぬ ね の は ば ぱ ひ び ぴ ふ ぶ ぷ へ べ ぺ ほ ぼ ぽ ま み む め も ゃ や ゅ ゆ ょ よ ら り る れ ろ ゎ わ ゐ ゑ を ん".split(" ")
        # entrylist2 = []
        # # for hiragana in hiraganalist:
        # for hiragana in reversed(self.__class__.SJIS_HIRAGANA_LIST):
        #     sublist = [ v for v in self.entries if v[0].startswith(hiragana) ]
        #     if len(sublist) > 1:
        #         sublist.sort(key=lambda v: v[0].encode("shiftjis"), reverse=True)
        #         entrylist2.extend(sublist)

        # self.entries = entrylist2

        self.sort_entries_hiraganabased()


        logger.debug("load_entries(): Loaded {} entries.".format(len(self.entries)))
        logger.debug("load_entries(): Yomigana maximum byte length = {}".format(self.yomigana_maxlen))


    def sort_entries_hiraganabased(self) -> None:
        """
        エントリーを、あ → ん の方向で、各ひらがな内は逆順で並べる        
        """
        newlist:List[DataEntry_str] = []

        for hiragana in reversed(self.__class__.SJIS_HIRAGANA_LIST):
            sublist = [ v for v in self.entries if v.key.startswith(hiragana) ]
            if len(sublist) > 1:
                sublist.sort(key=lambda v: v.key.encode("shiftjis"), reverse=True)
                newlist.extend(sublist)

        self.entries = newlist


    def build_optimized_index(self, max_indexkey_charlength:int) -> List[Tuple[str, int]]:
        """
        最適化されたインデックスの項目を生成する。各エントリにアドレスは設定されない。
        事前にエントリーを逆順で整列すること。
        
        Returns:
            生成されたいんでっくくの統計情報 [ (キー, 該当エントリ数), ... ]
        """

        # ひとつのキーが保持するエントリーの最大数
        MAXIMUM_COUNT_IN_ONE_KEY = self.maximum_index_key_length
        assert(MAXIMUM_COUNT_IN_ONE_KEY > 0)

        # [ (key:str, count:int), ...]
        indexentries:List[List] = []

        # 1文字インデックスをあらかじめ生成しておく。
        # 不要ならばあとで消えるので、辞書にかかわらずすべて追加する
        for k in self.__class__.SJIS_HIRAGANA_LIST:
            indexentries.append([k, 0])

        logger.debug("Generate base index entry...")
        # 最大キー長をベースにインデックスをリストアップする
        for ent in self.entries:
            key = ent.key[0:max_indexkey_charlength+1]
            candidates = [ ient for ient in indexentries if ient[0] == key ]
            if len(candidates) < 1:
                # 新規追加
                indexentries.append([key, 1])
            else:
                # カウントを1だけ足す
                # indexentries[indexentries.index(key)][1] += 1
                existing_entry = candidates[0]
                existing_entry[1] += 1

        # インデックスを降順にする
        indexentries.sort(key=lambda v: v[0].encode(ENC_SHIFTJIS), reverse=True)

        logger.debug("Make a group in base index entry...")

        # 文字数を削りながら、2つ以上該当項目のあるグループへまとめていく
        i = 0
        while True:
            i += 1
            if i == len(indexentries):
                break

            elif indexentries[i][1] >= MAXIMUM_COUNT_IN_ONE_KEY:
                # 現在の操作対象のグループが既に上限に到達しているので、上位グループに統合しない
                continue

            # elif len(indexentries[i][0]) < 3:
            elif len(indexentries[i][0]) < 2:
                # 1文字のエントリは統合しようがないのでスキップ
                continue

            else:
                newkey = indexentries[i][0][0:-1]
                candidates = [ v for v in indexentries if v[0] == newkey ]
                if len(candidates) < 1:
                    # まだないので追加する
                    newentry = [ newkey, 1 ]
                    # 既存のエントリを消す
                    indexentries[i][0] = ""
                    indexentries[i][1] = -1

                    indexentries.append(newentry)
                    continue

                else:
                    # すでにあるので、閾値を見て追加する。閾値を超えていたらスキップ
                    existingentry = candidates[0]
                    sumofentries = existingentry[1] + indexentries[i][1]
                    if sumofentries > MAXIMUM_COUNT_IN_ONE_KEY:
                        # なにもせずこのエントリの処理は完了する
                        continue
                    else:
                        # 上位グループへ合流する
                        existingentry[1] += indexentries[i][1]
                        indexentries[i][0] = ""
                        indexentries[i][1] = -1
                        continue

        # 無効なエントリを消す
        indexentries = [ v for v in indexentries if len(v[0]) > 0 and v[1] > 0 ]

        logger.debug("Sort indexentries in reverse.")
        # インデックスを降順にする
        indexentries.sort(key=lambda v: v[0].encode("shift_jis"), reverse=True)

        # インデックスひとつひとつについて、包含するキーへ統合を試みる


        if True:
            logger.debug("Try to concatnate...")

            # for i in range(len(indexentries)):
            i = 0
            while True:
                i += 1
                # 途中でアイテムを追加するので、ループごとに長さをチェックする
                if i == len(indexentries):
                    break

                # currententry = indexentries[i]
                if indexentries[i][1] < 0:
                    # 消去された項目なのでスキップ（到達しないはず）
                    logger.warn("Should not be reached in get_indexentries_2()")
                    continue

                # parententry_to_merge = None
                prev_parententry_index_to_merge = -1

                if len(indexentries[i][0]) > 1:
                    for j in range(1, len(indexentries[i][0])):
                        force_merge = (j == (len(indexentries[i][0]) - 1))
                        # print("force_merge = True. j = {}, ref = {}".format(j, (len(indexentries[i][0]))))
                        parentkey = indexentries[i][0][0:-j]
                        parententry2 = [v for v in indexentries if v[0] == parentkey]
                        if len(parententry2) == 0:
                            # logger.warning("Parent entry not found for {} ({})".format(indexentries[i][0], parentkey))
                            continue
                        elif len(parententry2) > 1:
                            logger.warning("Duplicate index entry: {}".format(parentkey))

                        # logger.debug("Parent entry for \"{}\" is \"{}\"".format(indexentries[i][0], parentkey))

                        # parententry = parententry2[0]
                        parententry_index = indexentries.index(parententry2[0])

                        sumcount = indexentries[parententry_index][1] + indexentries[i][1]
                        # print("sumcount={}, parent={}, myself={}".format(sumcount,indexentries[parententry_index], indexentries[i]))
                        # print("{}".format("FULLFILLED" if (sumcount >= MAXIMUM_COUNT_IN_ONE_KEY) else "UNDER"))
                        if sumcount < MAXIMUM_COUNT_IN_ONE_KEY:
                            # 現在の親候補へマージする
                            # logger.debug("Merge \"{}\" to \"{}\" (merge to current candidate)".format(indexentries[i][0], indexentries[parententry_index][0]))
                            indexentries[parententry_index][1] += indexentries[i][1]
                            indexentries[i][0] = ""
                            indexentries[i][1] = -1
                            prev_parententry_index_to_merge = -1
                            break

                        elif sumcount > MAXIMUM_COUNT_IN_ONE_KEY:
                            # 現在の親候補だと最大数を超すので、直前の候補へマージする
                            if prev_parententry_index_to_merge >= 0:
                                # logger.debug("Merge \"{}\" to \"{}\" (merge to prev candidate)".format(indexentries[i][0], indexentries[prev_parententry_index_to_merge][0]))
                                indexentries[prev_parententry_index_to_merge][1] += indexentries[i][1]
                                indexentries[i][0] = ""
                                indexentries[i][1] = -1
                                prev_parententry_index_to_merge = -1
                                break
                            else:
                                # 最大数を超すが親候補がない場合
                                # logger.warning("Should not be reached. parententry_to_merge is None for \"{}\"".format(indexentries[i][0]))
                                # if len(indexentries[i][0]) > 2:
                                #     newentry = [ indexentries[i][0][0:-1], 1 ]
                                #     indexentries[i][0] = ""
                                #     indexentries[i][1] = -1
                                #     prev_parententry_index_to_merge = -1
                                #     indexentries.append(newentry)
                                #     logger.info("Created new entry: \"{}\"".format(indexentries[i][0][0:-1]))
                                #     sys.stdin.readline()
                                #     break
                                break

                        else:
                            # いったん候補としてキープし、さらに親を辿る
                            # parententry[1] = sumcount
                            # indexentries[i][0] = ""
                            # indexentries[i][1] = -1
                            # parententry_to_merge = parententry
                            # logger.debug("Parent candidate for \"{}\" is \"{}\" at {}".format(indexentries[i][0], indexentries[parententry_index][0], parententry_index))
                            prev_parententry_index_to_merge = parententry_index
                            continue


        print("Generated index entry list (not filtered, not sorted):")
        print("----" * 4)
        pprint(indexentries)
        print("----" * 4)

        indexentries = [ v for v in indexentries if len(v[0]) > 0 and v[1] > 0 ]

        print("Generated index entry list (FILTERED, not sorted):")
        print("----" * 4)
        pprint(indexentries)
        print("----" * 4)


        # hiraganalist = "ぁ あ ぃ い ぅ う ぇ え ぉ お か が き ぎ く ぐ け げ こ ご さ ざ し じ す ず せ ぜ そ ぞ た だ ち ぢ っ つ づ て で と ど な に ぬ ね の は ば ぱ ひ び ぴ ふ ぶ ぷ へ べ ぺ ほ ぼ ぽ ま み む め も ゃ や ゅ ゆ ょ よ ら り る れ ろ ゎ わ ゐ ゑ を ん".split(" ")
        indexlist2 = []
        # for hiragana in hiraganalist:
        # for hiragana in reversed(self.__class__.SJIS_HIRAGANA_LIST):
        for hiragana in self.__class__.SJIS_HIRAGANA_LIST:
            sublist = [ v for v in indexentries if v[0].startswith(hiragana) ]
            if len(sublist) > 0:
                sublist.sort(key=lambda v: v[0].encode("shiftjis"), reverse=True)
                indexlist2.extend(sublist)
        
        indexentries = indexlist2

        print("Generated index entry list (sorted):")
        print("----" * 4)
        pprint(indexentries)
        print("----" * 4)

        logger.debug("Generate list to return...")

        # 返り値となるリストを生成する
        # ret = [ v[0] for v in indexentries ]

        # print("----" * 4)
        # print("Generated list:")
        # # print(ret)
        # pprint([ v for v in indexentries if v[1] > 0])
        # print("----" * 4)

        # self.generated_indexentry_count = len(ret)

        # logger.debug("get_indexentries_2() done. {} index entries.".format(self.generated_indexentry_count))

        # return ret

        logger.debug("get_indexentries_2() done. {} index entries.".format(len(indexentries)))

        self.indexentries = [ IndexEntry_str(ent[0], 0) for ent in indexentries ]

        return indexentries



    def generate_index_entry_bytes(self, yomigana:bytes, address:int) -> bytes:
        if isinstance(yomigana, bytearray):
            yomigana = bytes(yomigana)
        assert(isinstance(yomigana, bytes))
        assert(len(yomigana) > 0)
        b = bytearray()
        b.extend([len(yomigana)])
        b.extend(yomigana)
        b.extend(Util.convert_3bytes_to_lebytes(address))
        return b


    def get_indexentries(self) -> List[str]:
        SPLIT_THRESHOLD = 500  # インデックスを2文字に増やす分岐点
        logger.info("get_indexentries(): SPLIT_THRESHOLD={}".format(SPLIT_THRESHOLD))
        gross_entry_count = len(self.entries) / SPLIT_THRESHOLD
        logger.info(" Gross index entry count: {} ({} henkanentries per an indexentry".format(gross_entry_count, len(self.entries) / gross_entry_count))
        indexentries:List[bytes] = []
        for ent in self.entries:
            # 最初は1文字だけで
            firstyomiganachar:str = ent[0][0]
            # firstyomiganacharbytes = firstyomiganachar.encode("shiftjis")
            if indexentries.count(firstyomiganachar) > 0:
                # すでに登録済みのインデックス値なのでスキップ
                continue
            else:
                # 未登録のインデックス値
                twochar_yomigana = ent[0][0:2]
                # twochar_yomigana_bytes = twochar_yomigana.encode("shiftjis")
                if indexentries.count(twochar_yomigana) > 0:
                    # すでに登録済みのインデックス値なのでスキップ
                    continue
                entrycount_1 = self.count_entry_with_cond(lambda candidate: candidate[0] == firstyomiganachar)
                if entrycount_1 > SPLIT_THRESHOLD:
                    # 閾値を超えるので、2文字のエントリにする
                    # logger.info("Over threshold at {}".format(ent))
                    if indexentries.count(twochar_yomigana) > 0:
                        # 登録済みなのでスキップ
                        pass
                    else:
                        # 2文字で登録する
                        logger.info("Add 2-char index entry: {}".format(twochar_yomigana))
                        indexentries.append(twochar_yomigana)
                else:
                    # 閾値に収まるので、1文字エントリで登録する
                    indexentries.append(firstyomiganachar)
                    

            # if len(indexentries) == 0:
            #     indexentries.append(firstyomiganachar)
            # else:
            #     indexentrybytes = indexentries[-1].encode("shiftjis")
            #     if firstyomiganacharbytes != indexentrybytes:
            #         indexentries.append(firstyomiganachar)

        # 50, 100=good?
        MERGE_THRESHOLD = 100  # インデックス項目を統合対象とする閾値
        removelist = []
        for indexentry in indexentries:
            if len(indexentry) == 1:
                # 1文字エントリは消さない
                continue
            cnt = self.count_entry_with_cond(lambda s: s.startswith(indexentry))
            if cnt < MERGE_THRESHOLD:
                parent_cnt = self.count_entry_with_cond(lambda s: s.startswith(indexentry[0]))
                if parent_cnt > SPLIT_THRESHOLD:
                    # 親となるエントリの候補数が多い場合、閾値を下げる（==マージの頻度を下げる）
                    if cnt < int(MERGE_THRESHOLD / 10):
                        removelist.append(indexentry)
                else:
                    removelist.append(indexentry)
        print("removelist=", removelist)

        # removelistの重複を削る
        removelist = list(set(removelist))
        # indexentriesからremovelistを削り、削った文の1文字エントリを追加する
        for removeentry in removelist:
            if indexentries.count(removeentry) > 0:
                indexentries.remove(removeentry)
                indexentries.append(removeentry[0])
        # indexentriesの重複を削る
        indexentries = list(set(indexentries))
        # indexentriesを降順にする
        indexentries.sort(reverse=True)

        logger.debug("get_indexenttries(): Generated {} indexed.".format(len(indexentries)))
        print(indexentries)
        return indexentries

    def get_indexentries_2(self, max_indexkey_charlength:int) -> List[str]:
        """
        インデックスのエントリーを生成する。キーの最大長（文字数）を指定できる。
        事前にエントリーを逆順で整列すること。
        """

        # ひとつのキーが保持するエントリーの最大数
        # MAXIMUM_COUNT_IN_ONE_KEY = 500
        # MAXIMUM_COUNT_IN_ONE_KEY = 300
        MAXIMUM_COUNT_IN_ONE_KEY = self.maximum_index_key_length
        assert(MAXIMUM_COUNT_IN_ONE_KEY > 0)

        indexentries:List[List] = []

        # 1文字インデックスをあらかじめ生成しておく。
        # 不要ならばあとで消えるので、辞書にかかわらずすべて追加する
        # predefined_keys = "ぁ	あ	ぃ	い	ぅ	う	ぇ	え	ぉ	お	か	が	き	ぎ	く	ぐ	け	げ	こ	ご	さ	ざ	し	じ	す	ず	せ	ぜ	そ	ぞ	た	だ	ち	ぢ	っ	つ	づ	て	で	と	ど	な	に	ぬ	ね	の	は	ば	ぱ	ひ	び	ぴ	ふ	ぶ	ぷ	へ	べ	ぺ	ほ	ぼ	ぽ	ま	み	む	め	も	ゃ	や	ゅ	ゆ	ょ	よ	ら	り	る	れ	ろ	ゎ	わ	ゐ	ゑ	を	ん".split("\t")
        for k in self.__class__.SJIS_HIRAGANA_LIST:
            indexentries.append([k, 0])

        # print("Predefined indexentry:")
        # print(indexentries)

        # print("Press Enter to continue...")
        # sys.stdin.readline()

        logger.debug("Generate base index entry...")
        # 最大キー長をベースにインデックスをリストアップする
        for ent in self.entries:
            key = ent.key[0:max_indexkey_charlength+1]
            candidates = [ v for v in indexentries if v[0] == key ]
            if len(candidates) < 1:
                # 新規追加
                indexentries.append([key, 1])
            else:
                # カウントを1だけ足す
                # indexentries[indexentries.index(key)][1] += 1
                candidates[0][1] += 1

        # インデックスを降順にする
        indexentries.sort(key=lambda v: v[0].encode("shift_jis"), reverse=True)

        logger.debug("Make a group in base index entry...")

        # 文字数を削りながら、2つ以上該当項目のあるグループへまとめていく
        i = 0
        while True:
            i += 1
            if i == len(indexentries):
                break
            # elif indexentries[i][1] > 1:
            elif indexentries[i][1] >= MAXIMUM_COUNT_IN_ONE_KEY:
                continue
            elif len(indexentries[i][0]) < 3:
                # logger.info("Skip \"{}\"".format(indexentries[i][0]))
                continue
            else:
                newkey = indexentries[i][0][0:-1]
                candidates = [ v for v in indexentries if v[0] == newkey ]
                if len(candidates) < 1:
                    # まだないので追加する
                    newentry = [ newkey, 1 ]
                    # 既存のエントリを消す
                    indexentries[i][0] = ""
                    indexentries[i][1] = -1

                    indexentries.append(newentry)
                    continue
                else:
                    # すでにあるので、閾値を見て追加する。閾値を超えていたらスキップ
                    existingentry = candidates[0]
                    sumofentries = existingentry[1] + indexentries[i][1]
                    if sumofentries > MAXIMUM_COUNT_IN_ONE_KEY:
                        # なにもせずこのエントリの処理は完了する
                        continue
                    else:
                        # 上位グループへ合流する
                        existingentry[1] += indexentries[i][1]
                        indexentries[i][0] = ""
                        indexentries[i][1] = -1
                        continue

        # インデックスを降順にする
        indexentries.sort(key=lambda v: v[0].encode("shift_jis"), reverse=True)

        # pprint([ v for v in indexentries if len(v[0]) < 2 ])
        # print("Press Enter to continue...")
        # sys.stdin.readline()
 
        # pprint(indexentries)
        # print("Press Enter to continue...")
        # sys.stdin.readline()

        # 無効なエントリを消す
        indexentries = [ v for v in indexentries if len(v[0]) > 0 and v[1] >= 1 ]

        # print("----" * 4)
        # pprint(indexentries)
        # print("----" * 4)
        # logger.debug("Press Enter to continue...")
        # sys.stdin.readline()

        logger.debug("Sort indexentries in reverse.")
        # インデックスを降順にする
        indexentries.sort(key=lambda v: v[0].encode("shift_jis"), reverse=True)

        # インデックスひとつひとつについて、包含するキーへ統合を試みる


        if True:
            logger.debug("Try to concatnate...")

            # for i in range(len(indexentries)):
            i = 0
            while True:
                i += 1
                # 途中でアイテムを追加するので、ループごとに長さをチェックする
                if i == len(indexentries):
                    break

                # currententry = indexentries[i]
                if indexentries[i][1] < 0:
                    # 消去された項目なのでスキップ（到達しないはず）
                    logger.warn("Should not be reached in get_indexentries_2()")
                    continue

                # parententry_to_merge = None
                prev_parententry_index_to_merge = -1

                if len(indexentries[i][0]) > 1:
                    for j in range(1, len(indexentries[i][0])):
                        force_merge = (j == (len(indexentries[i][0]) - 1))
                        # print("force_merge = True. j = {}, ref = {}".format(j, (len(indexentries[i][0]))))
                        parentkey = indexentries[i][0][0:-j]
                        parententry2 = [v for v in indexentries if v[0] == parentkey]
                        if len(parententry2) == 0:
                            # logger.warning("Parent entry not found for {} ({})".format(indexentries[i][0], parentkey))
                            continue
                        elif len(parententry2) > 1:
                            logger.warning("Duplicate index entry: {}".format(parentkey))

                        # logger.debug("Parent entry for \"{}\" is \"{}\"".format(indexentries[i][0], parentkey))

                        # parententry = parententry2[0]
                        parententry_index = indexentries.index(parententry2[0])

                        sumcount = indexentries[parententry_index][1] + indexentries[i][1]
                        # print("sumcount={}, parent={}, myself={}".format(sumcount,indexentries[parententry_index], indexentries[i]))
                        # print("{}".format("FULLFILLED" if (sumcount >= MAXIMUM_COUNT_IN_ONE_KEY) else "UNDER"))
                        if sumcount < MAXIMUM_COUNT_IN_ONE_KEY:
                            # 現在の親候補へマージする
                            # logger.debug("Merge \"{}\" to \"{}\" (merge to current candidate)".format(indexentries[i][0], indexentries[parententry_index][0]))
                            indexentries[parententry_index][1] += indexentries[i][1]
                            indexentries[i][0] = ""
                            indexentries[i][1] = -1
                            prev_parententry_index_to_merge = -1
                            break

                        elif sumcount > MAXIMUM_COUNT_IN_ONE_KEY:
                            # 現在の親候補だと最大数を超すので、直前の候補へマージする
                            if prev_parententry_index_to_merge >= 0:
                                # logger.debug("Merge \"{}\" to \"{}\" (merge to prev candidate)".format(indexentries[i][0], indexentries[prev_parententry_index_to_merge][0]))
                                indexentries[prev_parententry_index_to_merge][1] += indexentries[i][1]
                                indexentries[i][0] = ""
                                indexentries[i][1] = -1
                                prev_parententry_index_to_merge = -1
                                break
                            else:
                                # 最大数を超すが親候補がない場合
                                # logger.warning("Should not be reached. parententry_to_merge is None for \"{}\"".format(indexentries[i][0]))
                                # if len(indexentries[i][0]) > 2:
                                #     newentry = [ indexentries[i][0][0:-1], 1 ]
                                #     indexentries[i][0] = ""
                                #     indexentries[i][1] = -1
                                #     prev_parententry_index_to_merge = -1
                                #     indexentries.append(newentry)
                                #     logger.info("Created new entry: \"{}\"".format(indexentries[i][0][0:-1]))
                                #     sys.stdin.readline()
                                #     break
                                break

                        else:
                            # いったん候補としてキープし、さらに親を辿る
                            # parententry[1] = sumcount
                            # indexentries[i][0] = ""
                            # indexentries[i][1] = -1
                            # parententry_to_merge = parententry
                            # logger.debug("Parent candidate for \"{}\" is \"{}\" at {}".format(indexentries[i][0], indexentries[parententry_index][0], parententry_index))
                            prev_parententry_index_to_merge = parententry_index
                            continue


        print("Generated index entry list (not filtered, not sorted):")
        print("----" * 4)
        pprint(indexentries)
        print("----" * 4)

        indexentries = [ v for v in indexentries if len(v[0]) > 0 and v[1] > 0 ]

        print("Generated index entry list (FILTERED, not sorted):")
        print("----" * 4)
        pprint(indexentries)
        print("----" * 4)


        # hiraganalist = "ぁ あ ぃ い ぅ う ぇ え ぉ お か が き ぎ く ぐ け げ こ ご さ ざ し じ す ず せ ぜ そ ぞ た だ ち ぢ っ つ づ て で と ど な に ぬ ね の は ば ぱ ひ び ぴ ふ ぶ ぷ へ べ ぺ ほ ぼ ぽ ま み む め も ゃ や ゅ ゆ ょ よ ら り る れ ろ ゎ わ ゐ ゑ を ん".split(" ")
        indexlist2 = []
        # for hiragana in hiraganalist:
        for hiragana in reversed(self.__class__.SJIS_HIRAGANA_LIST):
            sublist = [ v for v in indexentries if v[0].startswith(hiragana) ]
            if len(sublist) > 0:
                sublist.sort(key=lambda v: v[0].encode("shiftjis"), reverse=True)
                indexlist2.extend(sublist)
        
        indexentries = indexlist2

        print("Generated index entry list (sorted):")
        print("----" * 4)
        pprint(indexentries)
        print("----" * 4)

        logger.debug("Generate list to return...")

        # 返り値となるリストを生成する
        ret = [ v[0] for v in indexentries ]

        # print("----" * 4)
        # print("Generated list:")
        # # print(ret)
        # pprint([ v for v in indexentries if v[1] > 0])
        # print("----" * 4)

        self.generated_indexentry_count = len(ret)

        logger.debug("get_indexentries_2() done. {} index entries.".format(self.generated_indexentry_count))

        return ret


    def generate_binary(self) -> bytearray:
        """
        内部データをもとにバイナリ形式で出力する
        """
        assert(len(self.entries) > 0)
        assert(len(self.indexentries) > 0)

        b:bytearray = bytearray()

        commentbytes = self.comment_str.encode(ENC_SHIFTJIS)

        print("Target entry {}".format(len(self.entries)))

        globalheader:bytearray = bytearray()

        # Magic bytes 'SKD'
        globalheader.extend(list('SKD'.encode("ascii")))
        # File length uint24 placeholder (including this header)
        globalheader.extend(Util.convert_uint24_to_bytes(0))
        # Comment byte length uint16
        globalheader.extend(Util.convert_uint16_to_bytes(len(commentbytes)))
        # Comment bytes
        globalheader.extend(commentbytes)
        # Yomigana max byte length uint16
        globalheader.extend(Util.convert_uint16_to_bytes(self.yomigana_maxlen))

        globalheader_length = len(globalheader)
        if globalheader_length != (10 + len(commentbytes)):
            logger.error("globalheader_length is not expected value: Expected {} but actual {}".format((10 + len(commentbytes)), globalheader_length))
            raise Exception("globalheader_length is not expected value")

        b.extend(globalheader)


        indexentries_bytes = bytearray()

        for indexentry in self.indexentries:
            indexentries_bytes.extend(indexentry.to_bytes())

        indexheader:bytearray = bytearray()

        # Magic bytes 'IDX'
        indexheader.extend(list('IDX'.encode("ascii")))
        # index length uint24
        indexheader.extend(Util.convert_uint24_to_bytes(len(indexentries_bytes)))
        b.extend(indexheader)

        b.extend(indexentries_bytes)


        datasection_data_bytes = bytearray()

        for entry in self.entries:
            datasection_data_bytes.extend(entry.to_bytes())

        # Magic code 'TBL'
        b.extend(bytes('TBL'.encode("shiftjis")))
        # uint24 placeholder
        b.extend(Util.convert_uint24_to_bytes(len(datasection_data_bytes)))
        # Data body
        b.extend(datasection_data_bytes)
        
        # Update File size
        totallen = len(b)
        b[3:6] = Util.convert_3bytes_to_lebytes(totallen)

        logger.info("generate_binary(): Generated {} bytes.".format(len(b)))

        return b


    def build_binary(self, comment:str="") -> int:
        self.new_content:bytearray = bytearray()

        commentbytes = comment.encode(ENC_SHIFTJIS)

        print("Target entry {}".format(len(self.entries)))

        globalheader:bytearray = bytearray()

        # Magic bytes 'SKD'
        globalheader.extend(list('SKD'.encode("ascii")))
        # File length uint24 placeholder (including this header)
        globalheader.extend(Util.convert_uint24_to_bytes(0))
        # Comment byte length uint16
        globalheader.extend(Util.convert_uint16_to_bytes(len(commentbytes)))
        # Comment bytes
        globalheader.extend(commentbytes)
        # Yomigana max byte length uint16
        globalheader.extend(Util.convert_uint16_to_bytes(self.yomigana_maxlen))

        globalheader_length = len(globalheader)
        if globalheader_length != (10 + len(commentbytes)):
            logger.error("globalheader_length is not expected value: Expected {} but actual {}".format((10 + len(commentbytes)), globalheader_length))
            raise Exception("globalheader_length is not expected value")

        self.new_content.extend(globalheader)

        
        indexentries = bytearray()

        # First entry '***' placeholder
        # indexentries.extend(self.generate_index_entry_bytes('***'.encode("shiftjis"), 0))
        # Normal entries placeholder
        logger.info("Use get_indexentries_2()")
        indexentry_strlist = self.get_indexentries_2(3)
        for entry in indexentry_strlist:
            indexentries.extend(self.generate_index_entry_bytes(entry.encode("shiftjis"), 0))

        indexheader:bytearray = bytearray()

        # Magic bytes 'IDX'
        indexheader.extend(list('IDX'.encode("ascii")))
        # index length uint24
        indexheader.extend(Util.convert_uint24_to_bytes(len(indexentries)))
        self.new_content.extend(indexheader)

        self.new_content.extend(indexentries)


        datasection_data_bytes = bytearray()

        for entry in self.entries:
            datasection_data_bytes.extend(entry.to_bytes())

        # Magic code 'TBL'
        self.new_content.extend(bytes('TBL'.encode("shiftjis")))
        # uint24 placeholder
        self.new_content.extend(Util.convert_uint24_to_bytes(len(datasection_data_bytes)))
        # Data body
        self.new_content.extend(datasection_data_bytes)
        
        # Update File size
        totallen = len(self.new_content)
        self.new_content[3:6] = Util.convert_3bytes_to_lebytes(totallen)

        logger.info("build_binary(): Generated {} bytes.".format(len(self.new_content)))


    def save(self, binarydict:bytes, filepath:str) -> None:
        with open(filepath, "wb") as f:
            f.write(binarydict)


    @staticmethod
    def is_alphabet(ch:str) -> bool:
        if ord('a') <= ord(ch) <= ord('z'):
            return True
        if ord('A') <= ord(ch) <= ord('Z'):
            return True
        return False


    def is_entry_to_be_removed(self, ent:DataEntry_str) -> bool:
        """
        辞書から除外すべき項目か否かを判定する
        """

        # ASCII文字からはじまるエントリはすべて除去
        if ord(' ') <= ord(ent.key[0]) <= ord('~'):
            return True
        elif ent.key[0] == ">":
            # '>'ではじまるエントリは特殊用途なので（普通の登録もある）、除去
            return True
        elif ent.key[-1] == ">":
            # キーが'>'で終わるエントリは特殊用途なので（普通の登録もある）、除去
            return True
        else:
            return False


    def value_indexentry_address(self, binarydict:bytearray, searchtarget:bytes, newaddress:Optional[int]=None) -> Optional[int]:
        # b:bytes= self.new_content
        b = binarydict
        start_address = 3  # 'SKD'
        totallen = Util.convert_lebytes_to_uint24(b[start_address:start_address+3])
        start_address += 3
        comment_length = Util.convert_lebytes_to_uint16(b[start_address:start_address+2])
        # logger.debug("get_first_entry_address(): comment_length={}".format(comment_length))
        start_address += 2 + comment_length  # uint16 + comment

        yomigana_maxlength = Util.convert_lebytes_to_uint16(b[start_address:start_address+2])
        # logger.debug("get_first_entry_address(): yomigana_maxlength={}".format(yomigana_maxlength))
        start_address += 2  # uint16
        
        start_address += 3  # 'IDX'
        indexlen = Util.convert_lebytes_to_uint24(b[start_address:start_address+3])
        indextail = start_address + indexlen
        start_address += 3  # uint24

        while start_address < indextail:
            yomiganalen = Util.convert_lebytes_to_uint8(b[start_address])
            start_address += 1
            yomigana = b[start_address:start_address+yomiganalen]
            start_address += yomiganalen
            current_address = Util.convert_lebytes_to_uint24(b[start_address:start_address+3])
            start_address += 3

            if yomigana != searchtarget:
                # logger.debug("yomigana,searchtarget={}, {}".format(yomigana, searchtarget))
                continue
            else:
                if newaddress is None:
                    # logger.debug("get_first_entry_address(): read address={}".format(current_address))
                    return current_address
                else:
                    start_address -= 3
                    b[start_address:start_address+3] = Util.convert_3bytes_to_lebytes(newaddress)
                    return newaddress

        # raise Exception("value_indexentry_address(): Not found.")
        logger.warn("value_indexentry_address(): Not found. \"{}\"".format(searchtarget))


    def search_address_firsthit(self, binarydict:bytearray, startaddress:int, prev_key:str, expr:Callable[[bytes], bool]) -> Tuple[Optional[int], str]:
        # b = self.new_content
        b = binarydict
        cur_addr = 0
        cur_addr += 3  # Magic number 'SKD'
        totallen = Util.convert_lebytes_to_uint24(b[cur_addr:cur_addr+3])
        cur_addr += 3  # File size
        commentlen = Util.convert_lebytes_to_uint16(b[cur_addr:cur_addr+2])
        cur_addr += 2  # Comment length
        cur_addr += commentlen  # Comment body
        cur_addr += 2  # Longest yomigana length
        cur_addr += 3  # Magic number 'TBL'
        indexlen = Util.convert_lebytes_to_uint24(b[cur_addr:cur_addr+3])
        cur_addr += 3  # Index length
        # logger.debug("Index length={}".format(indexlen))
        cur_addr += indexlen  # Skip index
        if b[cur_addr:cur_addr+3] != b'TBL':
            logger.error("Expected Magic number not found: actual {}".format(b[cur_addr:cur_addr+3]))
            raise Exception("Magic number not found. TBL")
        cur_addr += 3
        tablelen = Util.convert_lebytes_to_uint24(b[cur_addr:cur_addr+3])
        cur_addr += 3
        while cur_addr < totallen:
            try:
                current_entry_address = cur_addr
                yomiganalen = Util.convert_lebytes_to_uint8(b[cur_addr])
                cur_addr += 1
                yomigana = b[cur_addr:cur_addr+yomiganalen]
                cur_addr += yomiganalen
                candidatescount = Util.convert_lebytes_to_uint8(b[cur_addr])
                cur_addr += 1
                candidatestotallen = Util.convert_lebytes_to_uint16(b[cur_addr:cur_addr+2])
                cur_addr += 2
                candidatesbytes = b[cur_addr:cur_addr+candidatestotallen]
                cur_addr += candidatestotallen
                # terminatenul = b[cur_addr]
                # cur_addr += 1

                if cur_addr <= startaddress:
                    # 指定された開始アドレス以前（同じかそれ以前）なので、比較対象外
                    continue
                elif len(prev_key) > 0 and yomigana.startswith(prev_key.encode(ENC_SHIFTJIS)):
                    # 前回のキーとまだ一致しているので、読み飛ばす
                    continue

                # assert(terminatenul == 0x00)
                if expr(yomigana):
                    # logger.debug("Hit with {}".format(yomigana.decode("shiftjis", errors='ignore')))
                    # logger.debug("Hit with an entry at {}: ({}bytes) \"{}\" candidates={}({}bytes) \"{}\"".format(
                    #     current_entry_address, yomiganalen, yomigana.decode("shiftjis"),
                    #     candidatescount, candidatestotallen,
                    #     candidatesbytes.decode("shiftjis", errors="backslashreplace"))
                    # )
                    # return current_entry_address
                    return (current_entry_address, (yomigana.decode("shiftjis")))

                else:
                    # logger.debug("Not hit with {}".format(yomigana.decode("shiftjis", errors='ignore')))
                    pass
            except:
                print("Exception. cur_addr={}".format(cur_addr))
                print(b[cur_addr-0x10:cur_addr])
                raise

        return (None, None)


    def get_indexentries_from_binary(self, binarydict:bytes) -> List[Tuple[str, int]]:
        entries:List[Tuple[str, int]] = []
        # b = self.new_content
        b = binarydict
        addr = 0
        addr += 3 + 3 # 'SKD' + uint24
        commentlen = b[addr:addr+2]
        addr += 2
        addr += Util.convert_lebytes_to_uint16(commentlen)
        yomiganamaxlen = Util.convert_lebytes_to_uint16(b[addr:addr+2])
        addr += 2
        addr += 3 # 'IDX'
        indexlen = Util.convert_lebytes_to_uint24(b[addr:addr+3])
        indextail = addr + indexlen
        addr += 3
        while addr < indextail:
            indexentrylen = Util.convert_lebytes_to_uint8(b[addr])
            addr += 1
            indexentrybytes = b[addr:addr+indexentrylen]
            addr += indexentrylen
            indexentryaddr = Util.convert_lebytes_to_uint24(b[addr:addr+3])
            addr += 3
            # logger.debug("Index entry: \"{}\"({}bytes) ADDR:{}".format(indexentrybytes.decode("shiftjis"), indexentrylen, indexentryaddr))
            entries.append((indexentrybytes.decode("shiftjis"), indexentryaddr))
        return entries


    def write_correct_address_for_index(self, binarydict:bytearray) -> None:
        """
        インデックスの各項目に対して、最初に合致する変換候補のアドレスを埋め込む。
        """
        self.indexentry_key_and_addr:List[Tuple[str,int]] = []
        search_start_address:int = 0
        prev_key = ""
        for indexentry in self.get_indexentries_from_binary(binarydict):
            headaddr, henkanentry = self.search_address_firsthit(binarydict, search_start_address, prev_key, lambda v: v.startswith(indexentry[0].encode("shiftjis")))
            if headaddr is not None:
                # logger.debug("Address {} for {}".format(headaddr, indexentry[0]))
                self.value_indexentry_address(binarydict, indexentry[0].encode("shiftjis"), headaddr)
                search_start_address = headaddr
                prev_key = indexentry[0]
                self.indexentry_key_and_addr.append((indexentry[0], headaddr))
            else:
                logger.debug("Not found address {} for {}".format(headaddr, indexentry[0]))


    def sort_entries_indexbased(self) -> None:
        """
        生成済みのインデックスをもとに、変換候補を並び変える。
        あ → ん の方向へ、基本は降順で。ただし1文字のインデックスキーにひも付く変換候補群は昇順で。
        インデックスは長いものから順に並んでいる必要がある。
        """
        entries = [ copy.deepcopy(ent) for ent in self.entries ]
        newlist:List[DataEntry_str] = []
        for indexentry in self.indexentries:
            sublist:List[DataEntry_str] = [ ent for ent in entries if len(ent.key) > 0 and ent.key.startswith(indexentry.key) and len(ent.values) > 0 ]
            if len(indexentry.key) == 1:
                sublist.sort(key=lambda ent: ent.key, reverse=False)
            else:
                sublist.sort(key=lambda ent: ent.key, reverse=True)
            newlist.extend(copy.deepcopy(sublist))
            for ent in sublist:
                # すでにヒットしたエントリーは無効化する
                ent.key = ""
                ent.values = []
        self.entries = newlist


    def dump_entries_as_json(self, filepath:str) -> None:
        jsoncontent = "[\n"
        for entry in self.entries:
            jsoncontent += entry.to_json() + ",\n"
        jsoncontent += "]\n"
        with open(filepath, "w", encoding="utf-8") as f:
            f.write(jsoncontent)

    def dump_indexentrystats_as_json(self, filepath:str, stats:List[List]) -> None:
        import json
        jsoncontent = "[\n"
        for ent in stats:
            jsoncontent += json.dumps(ent, ensure_ascii=False) + ",\n"
        jsoncontent += "]\n"
        with open(filepath, "w", encoding="utf-8") as f:
            f.write(jsoncontent)

    def dump_indexentry_as_json(self, filepath:str) -> None:
        import json
        jsoncontent = "[\n"
        for ent in self.indexentry_key_and_addr:
            jsoncontent += json.dumps(ent, ensure_ascii=False) + ",\n"
        jsoncontent += "]\n"
        with open(filepath, "w", encoding="utf-8") as f:
            f.write(jsoncontent)

def main():
    logger.info("Start")
    start_time = time.time()
    
    # if len(sys.argv) < 2:
    #     source_filepath = "SKK-JISYO-ML-SJIS.txt"
    # else:
    if True:
        source_filepath = sys.argv[1]
        maximum_index_key_length = int(sys.argv[2])
    dest_filepath = source_filepath + "_SKKDICT-" + str(int(time.time()))

    print("Loading \"{}\"".format(source_filepath))
    print("  (Save to \"{}\")".format(dest_filepath))

    util = SkkDictBinaryConverter()

    util.maximum_index_key_length = maximum_index_key_length
    print("maximum_index_key_length = {}".format(maximum_index_key_length))

    util.load(source_filepath)
    util.set_comment("LICENSE:GPL2+")

    print("Load entry...")
    util.load_entries()
    print("OK.")

    print("Build optimized index...")
    indexstats = util.build_optimized_index(maximum_index_key_length)
    print("OK.")

    print("Dump index stats as json...")
    json_dest_filepath_3 = dest_filepath + "_Threshold" + str(util.maximum_index_key_length) + "-cnt" + str(len(indexstats)) + "_indexstats.json"
    util.dump_indexentrystats_as_json(json_dest_filepath_3, indexstats)
    print("OK.")

    print("Sort entry...")
    util.sort_entries_indexbased()
    print("OK.")

    print("Build binary...")
    # util.build_binary()
    skkbinarydict_bytearray = util.generate_binary()
    print("OK.")

    print("Finishing index...")
    util.write_correct_address_for_index(skkbinarydict_bytearray)
    print("OK.")

    # print("dump_index_statistic() ...")
    # util.dump_index_statistic()
    # print("OK.")

    print("Dump as json...")
    json_dest_filepath = dest_filepath + "_Threshold" + str(util.maximum_index_key_length) + "-cnt" + str(len(indexstats)) + "_dump.json"
    util.dump_entries_as_json(json_dest_filepath)
    print("OK.")

    print("Dump index as json...")
    json_dest_filepath_2 = dest_filepath + "_Threshold" + str(util.maximum_index_key_length) + "-cnt" + str(len(indexstats)) + "_index.json"
    util.dump_indexentry_as_json(json_dest_filepath_2)
    print("OK.")

    print("Save...")
    dest_filepath = dest_filepath + "_Threshold" + str(util.maximum_index_key_length) + "-cnt" + str(len(indexstats)) + ".BIN"
    util.save(skkbinarydict_bytearray, dest_filepath)
    print("OK.")

    logger.info("Finished in {:.2f}[msec]".format(time.time() - start_time))


if __name__ == '__main__':
    main()
