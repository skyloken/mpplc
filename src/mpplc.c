#include "mpplc.h"

/* str of each token */
char *tokenstr[NUMOFTOKEN + 1] = {
        "",
        "NAME", "program", "var", "array", "of", "begin", "end", "if", "then",
        "else", "procedure", "return", "call", "while", "do", "not", "or",
        "div", "and", "char", "integer", "boolean", "readln", "writeln", "true",
        "false", "NUMBER", "STRING", "+", "-", "*", "=", "<>", "<", "<=", ">",
        ">=", "(", ")", "[", "]", ":=", ".", ",", ":", ";", "read", "write", "break"
};

/* str of each ttype */
char *ttypestr[NUMOFTTYPE + 1] = {
        "", "", "", "integer", "char", "boolean", "array", "procedure"
};

FILE *output;

int init_output(char *filename) {
    output = fopen(strcat(filename, ".csl"), "w");
    if (output == NULL) {
        return -1;
    } else {
        return 0;
    }
}

int main(int nc, char *np[]) {

    if (nc < 2) {
        printf("File name id not given.\n");
        return 0;
    }

    // 入力ファイル初期化
    if (init_scan(np[1]) < 0) {
        printf("File %s can not open.\n", np[1]);
        return 0;
    }

    // 出力ファイル初期化
    if (init_output(strtok(np[1], ".")) < 0) {
        printf("Output file can not open.\n");
        return 0;
    }

    init_idtab();

    /* 構文解析 */
    token = scan();
    if (parse_program() == ERROR) {
        printf("line: %d\n", get_linenum());
    }
    printf("CASLII code was generated.\n");
    end_scan();
    fclose(output);

    return 0;
}

int error(char *mes) {
    printf("\nERROR: %s\n", mes);
    return (ERROR);
}

void error_and_exit(char *mes, int linenum) {
    printf("\nERROR: %s\n", mes);
    printf("line: %d\n", linenum);
    end_scan();
    fclose(output);
    exit(0);
}