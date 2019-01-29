#include "mpplc.h"

struct ID *idroot; /* Pointers to root of global & local symbol tables */
struct STRLABEL *strlabelroot;

/* Initialise the table */
void init_idtab() {
    idroot = NULL;
    strlabelroot = NULL;
}

/* 参照行を末尾に追加 */
void append_reflinenum(struct LINE **root, int reflinenum) {
    struct LINE *p;
    struct LINE *l;

    // 領域確保
    if ((l = (struct LINE *) malloc(sizeof(struct LINE))) == NULL) {
        printf("can not malloc in append_reflinenum\n");
        return;
    }

    l->reflinenum = reflinenum;
    l->nextlinep = NULL;
    if (*root != NULL) {
        // 末尾を検索
        for (p = *root; p->nextlinep != NULL; p = p->nextlinep);
        // 末尾に追加
        p->nextlinep = l;
    } else {
        *root = l;
    }

}

/* 名前の二重定義チェック */
int is_defined(char *name, char *procname) {
    struct ID *p;

    for (p = idroot; p != NULL; p = p->nextp) {
        // 同じ名前が存在
        if (strcmp(p->name, name) == 0) {
            if (p->procname == NULL && procname == NULL) {
                return 1;
            }

            if (p->procname != NULL && procname != NULL) {
                if (strcmp(p->procname, procname) == 0) {
                    return 1;
                }
            }
        }
    }
    return 0;

}

/* 最も辞書順が低い要素を返す */
struct ID *search_dict_least() {
    struct ID *p, *min_p;
    char name_str[MAXSTRSIZE];
    char min_name_str[MAXSTRSIZE];
    int first_flag = 0;

    for (p = idroot; p != NULL; p = p->nextp) {
        // min_pの初期化
        if (p->is_output) {
            continue;
        } else {
            if (first_flag == 0) first_flag = 1;
        }
        if (first_flag == 1) {
            min_p = p;
            if (min_p->procname == NULL) {
                strcpy(min_name_str, min_p->name);
            } else {
                sprintf(min_name_str, "%s:%s", min_p->name, min_p->procname);
            }
            first_flag = 2;
        }

        // 比較
        if (p->procname == NULL) {
            strcpy(name_str, p->name);
        } else {
            sprintf(name_str, "%s:%s", p->name, p->procname);
        }
        if (strcmp(name_str, min_name_str) < 0) {
            min_p = p;
            if (min_p->procname == NULL) {
                strcpy(min_name_str, min_p->name);
            } else {
                sprintf(min_name_str, "%s:%s", min_p->name, min_p->procname);
            }
        }
    }
    return min_p;
}

/* 全て出力されたかチェック */
int is_all_output() {
    struct ID *p;

    for (p = idroot; p != NULL; p = p->nextp) {
        if (!p->is_output) return 0;
    }
    return 1;
}

/* 新規の名前を登録 */
void register_name(char *name, char *procname, int deflinenum, int ispara) {
    struct ID *p;
    struct TYPE *t;
    char *cname;
    char *cprocname;

    if (is_defined(name, procname)) error_and_exit("Name redefined", deflinenum);

    // 領域確保
    if ((p = (struct ID *) malloc(sizeof(struct ID))) == NULL) {
        printf("can not malloc in register_name\n");
        return;
    }
    if ((t = (struct TYPE *) malloc(sizeof(struct TYPE))) == NULL) {
        printf("can not malloc in register_name\n");
        return;
    }
    if ((cname = (char *) malloc(strlen(name) + 1)) == NULL) {
        printf("can not malloc in register_name\n");
        return;
    }
    strcpy(cname, name);
    if (procname != NULL) {
        if ((cprocname = (char *) malloc(strlen(procname) + 1)) == NULL) {
            printf("can not malloc in register_name\n");
            return;
        }
        strcpy(cprocname, procname);
    } else cprocname = NULL;

    // 情報登録
    p->name = cname;
    p->procname = cprocname;
    p->itp = t;
    p->itp->ttype = TPNULL;
    p->ispara = ispara;
    p->deflinenum = deflinenum;
    p->is_output = 0;
    p->irefp = NULL;
    p->iparap = NULL;
    p->nextp = idroot;
    idroot = p;

}

/* 名前の型情報を登録 */
void register_type_to_name(int ttype, int array_size, int element_type) {
    struct ID *p;

    // 型情報がない名前に対して型情報登録
    for (p = idroot; p != NULL; p = p->nextp) {
        if (p->itp->ttype == TPNULL) {
            p->itp->ttype = ttype;
            if (ttype == TPARRAY) {
                if (array_size < 1) error_and_exit("Size of the array must be 1 or more", p->deflinenum);
                if (p->ispara) error_and_exit("Formal parameters type must be standard type", p->deflinenum);
                p->itp->arraysize = array_size;
                p->itp->element_type = element_type;
                fprintf(output, "$%s\tDS\t%d\n", p->name, array_size);
            } else if (p->ispara || p->procname != NULL) {
                fprintf(output, "$%s%%%s\tDC\t0\n", p->name, p->procname);
            } else if (ttype != TPPROC) {
                fprintf(output, "$%s\tDC\t0\n", p->name);
            }
        }
    }

}

/* 参照された行を登録 */
int register_reflinenum(char *name, char *procname, int reflinenum) {
    struct ID *p;
    struct ID *p_tmp = NULL;
    int type;

    // 変数を検索
    for (p = idroot; p != NULL; p = p->nextp) {
        if (procname == NULL) {
            // プログラム内
            if (strcmp(p->name, name) == 0 && p->procname == NULL) {
                p_tmp = p;
                break;
            }
        } else {
            // 副プログラム内
            if (strcmp(p->name, name) == 0) {
                if (p->procname == NULL) {
                    // グローバル変数の場合，一旦記憶
                    p_tmp = p;
                } else if (strcmp(p->procname, procname) == 0) {
                    // ローカル変数が存在した場合，優先
                    p_tmp = p;
                    break;
                }
            }
        }

    }

    if (p_tmp != NULL) {
        type = p_tmp->itp->ttype;
        append_reflinenum(&(p_tmp->irefp), reflinenum);
        ele_type = p_tmp->itp->element_type;
        if (p_tmp->itp->ttype == TPPROC) {
            // 再帰チェック
            if (procname != NULL && strcmp(p_tmp->name, procname) == 0)
                error_and_exit("Recursive call is impossible", reflinenum);
            f_para = p_tmp->iparap;
        }
        if (p_tmp->itp->ttype != TPPROC) {
            fprintf(output, "\t");
            // 仮引数の場合LD，引数ではない普通の変数の場合LAD
            if (p_tmp->ispara) {
                fprintf(output, "LD");
            } else {
                fprintf(output, "LAD");
            }
            fprintf(output, "\tgr1,$%s", p_tmp->name);
            if (p_tmp->procname != NULL) {
                fprintf(output, "%%%s", p_tmp->procname);
            }
            fprintf(output, "\n");
        }
    } else {
        // 名前の未定義
        error_and_exit("Undefined name", reflinenum);
    }

    return type;
}

/* 手続きの引数を登録 */
void register_procedure_parameter(char *procname) {
    struct ID *p;
    struct ID *procp;
    struct PARA *para;

    // 手続き名を検索
    for (p = idroot; p != NULL; p = p->nextp) {
        if (strcmp(p->name, procname) == 0 && p->itp->ttype == TPPROC) {
            procp = p;
        }
    }

    // 手続きの引数を登録
    for (p = idroot; p != NULL; p = p->nextp) {
        if (p->procname != NULL) {
            if (strcmp(p->procname, procname) == 0 && p->ispara == 1) {

                // 登録
                if ((para = (struct PARA *) malloc(sizeof(struct PARA))) == NULL) {
                    printf("can not malloc in register_procedure_parameter\n");
                    return;
                }
                para->ttype = p->itp->ttype;
                para->name = p->name;
                para->nextparap = procp->iparap;
                procp->iparap = para;

            }
        }
    }
}

/* 副プログラムの仮引数リストの先頭ポインタを取得 */
struct PARA *get_procedure_parameter(char *procname) {
    struct ID *p;

    for (p = idroot; p != NULL; p = p->nextp) {
        if (strcmp(p->name, procname) == 0) {
            return p->iparap;
        }
    }

    return NULL;
}

/* Output the registered data */
void print_idtab() {
    struct ID *p;
    struct LINE *l;
    struct PARA *para;
    char type_str[MAXSTRSIZE];
    char name_str[MAXSTRSIZE];

    printf("%-20s\t%-50s\t%s\t%s\n", "Name", "Type", "Def.", "Ref.");

    while (!is_all_output()) {

        p = search_dict_least();

        // 出力フォーマット設定
        if (p->itp->ttype == TPARRAY) {
            sprintf(type_str, "array [%d] of %s", p->itp->arraysize, ttypestr[p->itp->element_type]);
        } else {
            strcpy(type_str, ttypestr[p->itp->ttype]);
        }
        if (p->procname == NULL) {
            strcpy(name_str, p->name);
        } else {
            sprintf(name_str, "%s:%s", p->name, p->procname);
        }
        if (p->itp->ttype == TPPROC) {
            strcpy(type_str, ttypestr[p->itp->ttype]);
            if (p->iparap != NULL) {
                strcat(type_str, "(");
                for (para = p->iparap; para != NULL; para = para->nextparap) {
                    strcat(type_str, ttypestr[para->ttype]);
                    if (para->nextparap != NULL) {
                        strcat(type_str, ", ");
                    }
                }
                strcat(type_str, ")");
            }
        }

        // 出力
        printf("%-20s\t%-50s\t%d\t", name_str, type_str, p->deflinenum);
        if (p->irefp != NULL) {
            for (l = p->irefp; l != NULL; l = l->nextlinep) {
                if (l->nextlinep != NULL) {
                    printf("%d, ", l->reflinenum);
                } else {
                    printf("%d", l->reflinenum);
                }
            }
        }
        printf("\n");
        p->is_output = 1;

    }
}

/* Register string and label */
void register_strlabel(char *string) {
    struct STRLABEL *p, *q;

    if ((p = (struct STRLABEL *) malloc(sizeof(struct STRLABEL))) == NULL) {
        printf("can not malloc in register_strlabel\n");
        exit(0);
    }
    p->label = label++;
    strcpy(p->string, string);
    p->nextp = NULL;
    if (strlabelroot == NULL) {
        strlabelroot = p;
    } else {
        for (q = strlabelroot; q->nextp != NULL; q = q->nextp) {
        }
        q->nextp = p;
    }
}

/* Output registerd string and label */
void output_strlabel() {
    struct STRLABEL *p;

    for (p = strlabelroot; p != NULL; p = p->nextp) {
        if (strlen(p->string) == 0) {
            fprintf(output, "L%04d\tDC\t0\n", p->label);
        } else {
            fprintf(output, "L%04d\tDC\t'%s'\n", p->label, p->string);
        }
    }
}

/* Output CASLII library */
void output_library(void) {
    fprintf(output, CASLII_EOVF);
    fprintf(output, CASLII_EOVF1);
    fprintf(output, CASLII_E0DIV);
    fprintf(output, CASLII_E0DIV1);
    fprintf(output, CASLII_EROV);
    fprintf(output, CASLII_EROV1);
    fprintf(output, CASLII_WRITECHAR);
    fprintf(output, CASLII_WRITESTR);
    fprintf(output, CASLII_BOVFCHECK);
    fprintf(output, CASLII_BOVFLEVEL);
    fprintf(output, CASLII_WRITEINT);
    fprintf(output, CASLII_MMINT);
    fprintf(output, CASLII_WRITEBOOL);
    fprintf(output, CASLII_WBTRUE);
    fprintf(output, CASLII_WBFALSE);
    fprintf(output, CASLII_WRITELINE);
    fprintf(output, CASLII_FLUSH);
    fprintf(output, CASLII_READCHAR);
    fprintf(output, CASLII_READINT);
    fprintf(output, CASLII_READLINE);
    fprintf(output, CASLII_ONE);
    fprintf(output, CASLII_SIX);
    fprintf(output, CASLII_TEN);
    fprintf(output, CASLII_SPACE);
    fprintf(output, CASLII_MINUS);
    fprintf(output, CASLII_TAB);
    fprintf(output, CASLII_ZERO);
    fprintf(output, CASLII_NINE);
    fprintf(output, CASLII_NEWLINE);
    fprintf(output, CASLII_INTBUF);
    fprintf(output, CASLII_OBUFSIZE);
    fprintf(output, CASLII_IBUFSIZE);
    fprintf(output, CASLII_INP);
    fprintf(output, CASLII_OBUF);
    fprintf(output, CASLII_IBUF);
    fprintf(output, CASLII_RPBBUF);
}