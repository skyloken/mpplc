//
// Created by ken on 2018/10/16.
//
#include "mpplc.h"

/* プライベート関数 */
static int is_alphabet(char);

static int is_digit(char);

/* グローバル変数 */
FILE *fp;
int num_attr;
char string_attr[MAXSTRSIZE];
int line_num;

/* keyword list */
struct KEY key[KEYWORDSIZE] = {
        {"and",       TAND},
        {"array",     TARRAY},
        {"begin",     TBEGIN},
        {"boolean",   TBOOLEAN},
        {"break",     TBREAK},
        {"call",      TCALL},
        {"char",      TCHAR},
        {"div",       TDIV},
        {"do",        TDO},
        {"else",      TELSE},
        {"end",       TEND},
        {"false",     TFALSE},
        {"if",        TIF},
        {"integer",   TINTEGER},
        {"not",       TNOT},
        {"of",        TOF},
        {"or",        TOR},
        {"procedure", TPROCEDURE},
        {"program",   TPROGRAM},
        {"read",      TREAD},
        {"readln",    TREADLN},
        {"return",    TRETURN},
        {"then",      TTHEN},
        {"true",      TTRUE},
        {"var",       TVAR},
        {"while",     TWHILE},
        {"write",     TWRITE},
        {"writeln",   TWRITELN}
};

/* 初期化関数 */
int init_scan(char *filename) {

    // 行番号
    line_num = 0;

    // Open file
    fp = fopen(filename, "r");
    if (fp == NULL) {
        return -1;
    } else {
        return 0;
    }

}

/* トークンを1つスキャンする関数 */
int scan() {
    // 初回は行番号を進める
    if (line_num == 0) line_num++;

    // 文字用バッファ
    char cbuf;
    char strbuf[MAXSTRSIZE];

    // 一時保存用
    char tmp;

    // エラーメッセージ
    char error_mes[] = "This file has an invalid token.";

    int i;
    while (1) {
        // 文字列バッファを初期化
        memset(strbuf, '\0', MAXSTRSIZE);

        // 文字列を読み込み
        cbuf = (char) fgetc(fp);

        // EOFの場合は終了
        if (cbuf == EOF) {
            return -1;
        }

        // 空白かタブの場合は読み飛ばす
        if (cbuf == ' ' || cbuf == '\t' || cbuf == '\r') {
            continue;
        }

        // 改行の場合
        if (cbuf == '\n') {
            line_num++;
            continue;
        }

        if (is_alphabet(cbuf)) {
            // 英字の場合

            // 英数字が続く限り読み込む
            for (i = 0; i < MAXSTRSIZE; i++) {
                if (is_alphabet(cbuf) || is_digit(cbuf)) {
                    // 1023文字より多い場合
                    if (i == (MAXSTRSIZE - 1)) {
                        error_and_exit(error_mes, get_linenum());
                        return -1;
                    } else {
                        strbuf[i] = cbuf;
                    }
                } else {
                    break;
                }
                cbuf = (char) fgetc(fp);
            }
            ungetc(cbuf, fp);

            // キーワードかどうかを判別し，トークンを返す
            for (i = 0; i < KEYWORDSIZE; i++) {
                if (strcmp(key[i].keyword, strbuf) == 0) {
                    return key[i].keytoken;
                }
            }

            // キーワードでなければ名前
            strcpy(string_attr, strbuf);
            return TNAME;

        } else if (is_digit(cbuf)) {
            // 数字の場合

            // 数字が続く限り読み込む
            for (i = 0; i < MAXSTRSIZE; i++) {
                if (is_digit(cbuf)) {
                    // 1023文字より多い場合
                    if (i == (MAXSTRSIZE - 1)) {
                        error_and_exit(error_mes, get_linenum());
                        return -1;
                    } else {
                        strbuf[i] = cbuf;
                    }
                } else {
                    break;
                }
                cbuf = (char) fgetc(fp);
            }
            ungetc(cbuf, fp);

            // 32767より大きい場合
            if (atoi(strbuf) > 32767) {
                error_and_exit(error_mes, get_linenum());
                return -1;
            } else {
                num_attr = atoi(strbuf);
            }
            strcpy(string_attr, strbuf);
            return TNUMBER;

        } else if (cbuf == '\'') {
            // 文字列要素
            for (i = 0; i < MAXSTRSIZE; i++) {

                cbuf = (char) fgetc(fp);

                // 文字列中に改行かEOFがあった場合、エラー
                if (cbuf == EOF || cbuf == '\n') {
                    error_and_exit(error_mes, get_linenum());
                    return -1;
                }

                // 'の場合
                tmp = cbuf;
                if (cbuf == '\'') {
                    cbuf = (char) fgetc(fp);
                    if (cbuf == '\'') {
                        // 'が連続していた場合，アポストロフィ扱いとなり、スキップ
                        strbuf[i] = tmp;
                        strbuf[++i] = cbuf;
                        continue;
                    } else {
                        // 'でなければ，文字列終了
                        ungetc(cbuf, fp);
                        break;
                    }
                }
                // 1023文字より多い場合
                if (i == (MAXSTRSIZE - 1)) {
                    error_and_exit(error_mes, get_linenum());
                    return -1;
                } else {
                    strbuf[i] = tmp;
                }
            }
            strcpy(string_attr, strbuf);
            return TSTRING;

        } else if (cbuf == '/') {
            // 注釈要素 /* */
            cbuf = (char) fgetc(fp); // '*'を読む

            // '/'のみの場合
            if (cbuf != '*') {
                error_and_exit(error_mes, get_linenum());
                return -1;
            }

            while (1) {
                cbuf = (char) fgetc(fp);

                // 注釈中に改行かEOFがあった場合、エラー
                if (cbuf == EOF || cbuf == '\n') {
                    error_and_exit(error_mes, get_linenum());
                    return -1;
                }

                // '*/'で注釈要素終了
                if (cbuf == '*') {
                    cbuf = (char) fgetc(fp);
                    if (cbuf == '/') break;
                    ungetc(cbuf, fp);
                }
            }
            continue;

        } else if (cbuf == '{') {
            // 注釈要素 { }
            do {
                cbuf = (char) fgetc(fp);

                // 注釈中に改行かEOFがあった場合、エラー
                if (cbuf == EOF || cbuf == '\n') {
                    error_and_exit(error_mes, get_linenum());
                    return -1;
                }

            } while (cbuf != '}');
            continue;

        } else {
            // 記号要素

            tmp = cbuf;
            // <=, <>
            if (cbuf == '<') {
                cbuf = (char) fgetc(fp);
                if (cbuf == '=') return TLEEQ;
                if (cbuf == '>') return TNOTEQ;
                ungetc(cbuf, fp);
            }
            // >=
            if (cbuf == '>') {
                cbuf = (char) fgetc(fp);
                if (cbuf == '=') return TGREQ;
                ungetc(cbuf, fp);
            }
            // :=
            if (cbuf == ':') {
                cbuf = (char) fgetc(fp);
                if (cbuf == '=') return TASSIGN;
                ungetc(cbuf, fp);
            }
            // それ以外
            cbuf = tmp;
            for (i = TPLUS; i <= TSEMI; i++) {
                if (i == TLEEQ || i == TGREQ || i == TASSIGN || i == TNOTEQ) continue;
                if (cbuf == tokenstr[i][0]) return i;
            }

            // 字句や分離子を構成しない文字が出現
            error_and_exit(error_mes, get_linenum());
            return -1;
        }
    }
}

/* 行番号関数 */
int get_linenum() {
    return line_num;
}

/* 終了処理関数 */
void end_scan() {
    fclose(fp);
}

/* アルファベット判定 */
static int is_alphabet(char c) {
    if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z')) {
        return 1;
    } else {
        return 0;
    }
}

/* 数字判定 */
static int is_digit(char c) {
    if (c >= '0' && c <= '9') {
        return 1;
    } else {
        return 0;
    }
}