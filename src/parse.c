//
// Created by ken on 2018/11/08.
//

#include "mpplc.h"

/* グローバル変数 */
int token;
char name[MAXSTRSIZE];
char procname[MAXSTRSIZE];
int linenum;
int ttype;
int array_size;
int element_type;
int ele_type;
int label = 1;
int break_label;

int ispara = 0;
int iscallpara = 0;
int is_local = 0;
int is_call = 0;
int is_opr = 0;

struct PARA *f_para;

/* プロトタイプ宣言 */
int block();

int variable_declaration();

int variable_names();

int variable_name();

int type();

int standard_type();

int array_type();

int subprogram_declaration();

int procedure_name();

int formal_parameters();

int compound_statement();

int statement();

int condition_statement();

int iteration_statement();

int exit_statement();

int call_statement();

int expressions();

int return_statement();

int assignment_statement();

int left_part();

int variable();

int expression();

int simple_expression();

int term();

int factor();

int constant();

int multiplicative_operator();

int additive_operator();

int relational_operator();

int input_statement();

int output_statement();

int output_format();

int empty_statement();

int token_to_ttype(int token);

int is_standard_type(int type);

void reverse_show_parameter(struct PARA *);

/* プログラム */
int parse_program() {

    if (token != TPROGRAM) return (error("Keyword 'program' is not found"));
    token = scan();
    if (token != TNAME) return (error("Program name is not found"));

    fprintf(output, "$$%s\tSTART\n", string_attr);
    fprintf(output, "\tLAD\tgr0,0\n");
    fprintf(output, "\tCALL\tL%04d\n", label++);
    fprintf(output, "\tCALL\tFLUSH\n");
    fprintf(output, "\tSVC\t0\n");

    token = scan();
    if (token != TSEMI) return (error("Semicolon is not found"));
    token = scan();
    if (block() == ERROR) return (ERROR);
    if (token != TDOT) return (error("Period is not found at the end of program"));
    token = scan();
    output_strlabel();
    output_library();
    fprintf(output, "\tEND\n");
    return (NORMAL);
}

/* ブロック */
int block() {
    while (token == TVAR || token == TPROCEDURE) {
        switch (token) {
            case TVAR:
                if (variable_declaration() == ERROR) return ERROR;
                break;
            case TPROCEDURE:
                if (subprogram_declaration() == ERROR) return ERROR;
                break;
        }
    }
    fprintf(output, "L0001\n");
    if (compound_statement() == ERROR) return ERROR;
    fprintf(output, "\tRET\n");
    return NORMAL;
}

/* 変数宣言部 */
int variable_declaration() {
    if (token != TVAR) return (error("Keyword 'var' is not found"));
    token = scan();
    if (variable_names() == ERROR) return ERROR;
    if (token != TCOLON) return error("Symbol ':' is not found");
    token = scan();
    if (type() == ERROR) return ERROR;
    register_type_to_name(ttype, array_size, element_type);
    if (token != TSEMI) return error("Symbol ';' is not found");
    token = scan();
    while (token == TNAME) {
        if (variable_names() == ERROR) return ERROR;
        if (token != TCOLON) return error("Symbol ':' is not found");
        token = scan();
        if (type() == ERROR) return ERROR;
        register_type_to_name(ttype, array_size, element_type);
        if (token != TSEMI) return error("Symbol ';' is not found");
        token = scan();
    }
    return NORMAL;
}

/* 変数の並び */
int variable_names() {
    if (variable_name() == ERROR) return ERROR;
    if (is_local) {
        register_name(name, procname, linenum, ispara);
    } else {
        register_name(name, NULL, linenum, ispara);
    }
    while (token == TCOMMA) {
        token = scan();
        if (variable_name() == ERROR) return ERROR;
        if (is_local) {
            register_name(name, procname, linenum, ispara);
        } else {
            register_name(name, NULL, linenum, ispara);
        }
    }
    return NORMAL;
}

/* 変数名 */
int variable_name() {
    if (token != TNAME) return error("Variable name is not found");
    strcpy(name, string_attr);
    linenum = get_linenum();
    token = scan();
    return NORMAL;
}

/* 型 */
int type() {
    switch (token) {
        case TINTEGER:
        case TBOOLEAN:
        case TCHAR:
            if (standard_type() == ERROR) return ERROR;
            break;
        case TARRAY:
            if (array_type() == ERROR) return ERROR;
            break;
        default:
            error("Type is not found");
    }

    return NORMAL;
}

/* 標準型 */
int standard_type() {
    ttype = token_to_ttype(token);
    if (token != TINTEGER && token != TBOOLEAN && token != TCHAR)
        return error("Keyword 'integer' or 'boolean' or 'char' is not found");
    token = scan();
    return NORMAL;
}

/* 配列型 */
int array_type() {
    if (token != TARRAY) return error("Keyword 'array' is not found");
    token = scan();
    if (token != TLSQPAREN) return error("Symbol '[' is not found");
    token = scan();
    array_size = num_attr;
    if (token != TNUMBER) return error("Number is not found");
    token = scan();
    if (token != TRSQPAREN) return error("Symbol ']' is not found");
    token = scan();
    if (token != TOF) return error("Keyword 'of' is not found");
    token = scan();
    element_type = token_to_ttype(token);
    if (standard_type() == ERROR) return ERROR;
    ttype = TPARRAY;
    return NORMAL;
}

/* 副プログラム宣言 */
int subprogram_declaration() {
    is_local = 1;
    if (token != TPROCEDURE) return error("Keyword 'procedure' is not found");
    token = scan();
    if (procedure_name() == ERROR) return ERROR;
    register_name(procname, NULL, linenum, 0);
    register_type_to_name(TPPROC, 0, 0);
    if (token == TLPAREN) {
        if (formal_parameters() == ERROR) return ERROR;
    }
    register_procedure_parameter(procname);
    if (token != TSEMI) return error("Symbol ';' is not found");
    token = scan();
    if (token == TVAR) {
        if (variable_declaration() == ERROR) return ERROR;
    }

    fprintf(output, "$%s\n", procname);
    struct PARA *p = get_procedure_parameter(procname);
    if (p != NULL) {
        fprintf(output, "\tPOP\tgr2\n");
        // 逆順に引数を出力
        reverse_show_parameter(p);
        fprintf(output, "\tPUSH\t0,gr2\n");
    }

    if (compound_statement() == ERROR) return ERROR;
    fprintf(output, "\tRET\n");
    if (token != TSEMI) return error("Symbol ';' is not found");
    token = scan();
    is_local = 0;
    return NORMAL;
}

/* 手続き名 */
int procedure_name() {
    if (token != TNAME) return error("Procedure name is not found");
    if (is_call) {
        // 呼び出し時
        if (is_local) {
            register_reflinenum(string_attr, procname, get_linenum());
        } else {
            register_reflinenum(string_attr, NULL, get_linenum());
        }

    } else {
        // 宣言時
        strcpy(procname, string_attr);
        linenum = get_linenum();
    }
    token = scan();
    return NORMAL;
}

/* 仮引数部 */
int formal_parameters() {
    ispara = 1;
    if (token != TLPAREN) return error("Symbol '(' is not found");
    token = scan();
    if (variable_names() == ERROR) return ERROR;
    if (token != TCOLON) return error("Symbol ':' is not found");
    token = scan();
    if (type() == ERROR) return ERROR;
    register_type_to_name(ttype, array_size, element_type);
    while (token == TSEMI) {
        token = scan();
        if (variable_names() == ERROR) return ERROR;
        if (token != TCOLON) return error("Symbol ':' is not found");
        token = scan();
        if (type() == ERROR) return ERROR;
        register_type_to_name(ttype, array_size, element_type);
    }
    if (token != TRPAREN) return error("Symbol '(' is not found");
    token = scan();
    ispara = 0;
    return NORMAL;
}

/* 複合文 */
int compound_statement() {
    if (token != TBEGIN) return error("Keyword 'begin' is not found");
    token = scan();
    if (statement() == ERROR) return ERROR;
    while (token == TSEMI) {
        token = scan();
        if (statement() == ERROR) return ERROR;
    }
    if (token != TEND) return error("Keyword 'end' is not found");
    token = scan();
    return NORMAL;
}

/* 文 */
int statement() {
    int break_label_tmp;

    switch (token) {
        case TNAME:
            if (assignment_statement() == ERROR) return ERROR;
            break;
        case TIF:
            if (condition_statement() == ERROR) return ERROR;
            break;
        case TWHILE:
            break_label_tmp = break_label;
            if (iteration_statement() == ERROR) return ERROR;
            break_label = break_label_tmp;
            break;
        case TBREAK:
            if (exit_statement() == ERROR) return ERROR;
            fprintf(output, "\tJUMP\tL%04d\n", break_label);
            break;
        case TCALL:
            if (call_statement() == ERROR) return ERROR;
            break;
        case TRETURN:
            if (return_statement() == ERROR) return ERROR;
            fprintf(output, "\tRET\n");
            break;
        case TREAD:
        case TREADLN:
            if (input_statement() == ERROR) return ERROR;
            break;
        case TWRITE:
        case TWRITELN:
            if (output_statement() == ERROR) return ERROR;
            break;
        case TBEGIN:
            if (compound_statement() == ERROR) return ERROR;
            break;
        default:
            if (empty_statement() == ERROR) return ERROR;
            break;
    }
    return NORMAL;
}

/* 分岐文 */
int condition_statement() {
    int type;
    int label1, label2;

    if (token != TIF) return error("Keyword 'if' is not found");
    token = scan();
    if ((type = expression()) == ERROR) return ERROR;
    if (type != TPBOOL) return error("Conditional expression of condition statement must be boolean");

    label1 = label++;
    fprintf(output, "\tCPA\tgr1,gr0\n");
    fprintf(output, "\tJZE\tL%04d\n", label1);

    if (token != TTHEN) return error("Keyword 'then' is not found");
    token = scan();
    if (statement() == ERROR) return ERROR;

    if (token == TELSE) {

        label2 = label++;
        fprintf(output, "\tJUMP\tL%04d\n", label2);
        fprintf(output, "L%04d\n", label1);

        token = scan();
        if (statement() == ERROR) return ERROR;

        fprintf(output, "L%04d\n", label2);

    } else {
        fprintf(output, "L%04d\n", label1);
    }
    return NORMAL;
}

/* 繰り返し文 */
int iteration_statement() {
    int label1;

    int type;
    if (token != TWHILE) return error("Keyword 'while' is not found");

    label1 = label;
    fprintf(output, "L%04d\n", label1);
    label = label + 2;

    token = scan();
    if ((type = expression()) == ERROR) return ERROR;
    if (type != TPBOOL) return error("Conditional expression of iteration statement must be boolean");

    fprintf(output, "\tCPA\tgr1,gr0\n");
    fprintf(output, "\tJZE\tL%04d\n", label1 + 1);

    if (token != TDO) return error("Keyword 'do' is not found");
    token = scan();

    if (token == TSEMI || token == TEND) {
        fprintf(output, "L%04d\n", label1 + 1);
        return (NORMAL);
    }
    break_label = label1 + 1;

    if (statement() == ERROR) return ERROR;

    fprintf(output, "\tJUMP\tL%04d\n", label1);
    fprintf(output, "L%04d\n", label1 + 1);

    return NORMAL;
}

/* 脱出文 */
int exit_statement() {
    if (token != TBREAK) return error("Keyword 'break' is not found");
    token = scan();
    return NORMAL;
}

/* 手続き呼び出し文 */
int call_statement() {
    is_call = 1;
    int no_arg = 1;
    char pname[MAXSTRSIZE];

    if (token != TCALL) return error("Keyword 'call' is not found");
    token = scan();
    if (procedure_name() == ERROR) return ERROR;
    strcpy(pname, string_attr);
    if (token == TLPAREN) {
        no_arg = 0;
        token = scan();
        iscallpara = 1;
        if (expressions() == ERROR) return ERROR;
        iscallpara = 0;
        if (token != TRPAREN) return error("Symbol ')' is not found");
        token = scan();
    }
    if (no_arg && f_para != NULL) return error("Number of procedure arguments do not match");
    fprintf(output, "\tCALL\t$%s\n", pname);
    is_call = 0;
    return NORMAL;
}

/* 式の並び */
int expressions() {
    int type;
    int first;
    first = token;
    is_opr = 0;

    if ((type = expression()) == ERROR) return ERROR;
    if (f_para == NULL) return error("Number of procedure arguments do not match");
    if (type != f_para->ttype) return error("Procedure argument types do not match");

    if (first == TNAME && is_opr == 0) {
        fprintf(output, "\tPUSH\t0,gr1\n");
    } else {
        fprintf(output, "\tLAD\tgr2,L%04d\n", label);
        fprintf(output, "\tST\tgr1,0,gr2\n");
        fprintf(output, "\tPUSH\t0,gr2\n");
        register_strlabel("");
    }

    while (token == TCOMMA) {
        if (f_para->nextparap == NULL) return error("Number of procedure arguments do not match");
        f_para = f_para->nextparap;
        token = scan();
        first = token;
        is_opr = 0;
        if ((type = expression()) == ERROR) return ERROR;
        if (type != f_para->ttype) return error("Procedure argument types do not match");

        if (first == TNAME && is_opr == 0) {
            fprintf(output, "\tPUSH\t0,gr1\n");
        } else {
            fprintf(output, "\tLAD\tgr2,L%04d\n", label);
            fprintf(output, "\tST\tgr1,0,gr2\n");
            fprintf(output, "\tPUSH\t0,gr2\n");
            register_strlabel("");
        }
    }
    if (f_para->nextparap != NULL) return error("Number of procedure arguments do not match");
    return NORMAL;
}

/* 戻り文 */
int return_statement() {
    if (token != TRETURN) return error("Keyword 'return' is not found");
    token = scan();
    return NORMAL;
}

/* 代入文 */
int assignment_statement() {
    int type1, type2;
    if ((type1 = left_part()) == ERROR) return ERROR;
    if (token != TASSIGN) return error("Symbol ':=' is not found");
    token = scan();
    fprintf(output, "\tPUSH\t0,gr1\n");
    if ((type2 = expression()) == ERROR) return ERROR;
    if (type1 != type2) return error("Left part and expression type must be same type");
    if (!(is_standard_type(type1) && is_standard_type(type2)))
        return error("Left part and expression type must be standard type");
    fprintf(output, "\tPOP\tgr2\n");
    fprintf(output, "\tST\tgr1,0,gr2\n");
    return NORMAL;
}

/* 左辺部 */
int left_part() {
    int type;
    if ((type = variable()) == ERROR) return ERROR;
    return type;
}

/* 変数 */
int variable() {
    int type;
    if (variable_name() == ERROR) return ERROR;
    if (is_local) {
        type = register_reflinenum(name, procname, linenum);
    } else {
        type = register_reflinenum(name, NULL, linenum);
    }
    if (token == TLSQPAREN) {
        int sub_type;
        if (type != TPARRAY) return error("When there is subscript, variable name type must be array");
        token = scan();
        fprintf(output, "\tPUSH\t0,gr1\n");
        if ((sub_type = expression()) == ERROR) return ERROR;
        if (sub_type != TPINT) return error("Array subscript type must be integer");

        fprintf(output, "\tCPA\tgr0,gr2\n");
        fprintf(output, "\tJPL\tL%04d\n", label);
        fprintf(output, "\tLAD\tgr2,%d\n", array_size);
        fprintf(output, "\tCPA\tgr1,gr2\n");
        fprintf(output, "\tJMI\tL%04d\n", label + 1);
        fprintf(output, "L%04d\n", label);
        fprintf(output, "\tCALL\tEROV\n");
        fprintf(output, "L%04d\n", label + 1);
        fprintf(output, "\tPOP\tgr2\n");
        fprintf(output, "\tADDA\tgr1,gr2\n");
        label = label + 2;

        if (token != TRSQPAREN) return error("Symbol ']' is not found");
        token = scan();
        type = ele_type;
    }
    return type;
}

/* 式 */
int expression() {
    int type;
    if ((type = simple_expression()) == ERROR) return ERROR;
    while (token == TEQUAL ||
           token == TNOTEQ ||
           token == TLE ||
           token == TLEEQ ||
           token == TGR ||
           token == TGREQ) {
        int type1 = type, type2;
        int opr = token;
        is_opr = 1;
        fprintf(output, "\tPUSH\t0,gr1\n");
        if (relational_operator() == ERROR) return ERROR;
        if ((type2 = simple_expression()) == ERROR) return ERROR;
        if (type1 != type2) return error("Operands of relational operator must be same type");
        if (!(is_standard_type(type1) && is_standard_type(type2)))
            return error("Operand of relational operator must be standard type");
        type = TPBOOL;

        fprintf(output, "\tPOP\tgr2\n");
        fprintf(output, "\tCPA\tgr2,gr1\n");
        switch (opr) {
            case TEQUAL:
                fprintf(output, "\tJZE\tL%04d\n\tLD\tgr1,gr0\n\tJUMP\tL%04d\nL%04d\n\tLAD\tgr1,1\nL%04d\n", label,
                        label + 1, label, label + 1);
                label = label + 2;
                break;
            case TNOTEQ:
                fprintf(output, "\tJNZ\tL%04d\n\tLD\tgr1,gr0\n\tJUMP\tL%04d\nL%04d\n\tLAD\tgr1,1\nL%04d\n", label,
                        label + 1, label, label + 1);
                label = label + 2;
                break;
            case TGR:
                fprintf(output, "\tJPL\tL%04d\n\tLD\tgr1,gr0\n\tJUMP\tL%04d\nL%04d\n\tLAD\tgr1,1\nL%04d\n", label,
                        label + 1, label, label + 1);
                label = label + 2;
                break;
            case TGREQ:
                fprintf(output,
                        "\tJZE\tL%04d\n\tJPL\tL%04d\n\tLD\tgr1,gr0\n\tJUMP\tL%04d\nL%04d\n\tLAD\tgr1,1\nL%04d\n", label,
                        label, label + 1, label, label + 1);
                label = label + 2;
                break;
            case TLE:
                fprintf(output, "\tJMI\tL%04d\n\tLD\tgr1,gr0\n\tJUMP\tL%04d\nL%04d\n\tLAD\tgr1,1\nL%04d\n", label,
                        label + 1, label, label + 1);
                label = label + 2;
                break;
            case TLEEQ:
                fprintf(output, "\tJPL\tL%04d\n\tLAD\tgr1,1\n\tJUMP\tL%04d\nL%04d\n\tLD\tgr1,gr0\nL%04d\n", label,
                        label + 1, label, label + 1);
                label = label + 2;
                break;
            default:
                exit(EXIT_FAILURE);
        }
    }
    return type;
}

/* 単純式 */
int simple_expression() {
    int is_right_ope = 0, type;
    int is_minus = 0;
    if (token == TPLUS || token == TMINUS) {
        is_right_ope = 1;
        is_opr = 1;
        if (token == TMINUS) is_minus = 1;
        token = scan();
    }
    if ((type = term()) == ERROR) return ERROR;
    if (is_right_ope && type != TPINT) return error("When there is '+' or '-', Type of left term must be integer");

    if (is_minus) {
        fprintf(output, "\tLAD\tgr2,65535\n\tSUBA\tgr2,gr1\n\tLAD\tgr1,1\n\tADDA\tgr1,gr2\n\tJOV\tEOVF\n");
    }
    while (token == TPLUS || token == TMINUS || token == TOR) {
        int type1 = type, type2, opr = token;
        is_opr = 1;
        fprintf(output, "\tPUSH\t0,gr1\n");
        if (additive_operator() == ERROR) return ERROR;
        if ((type2 = term()) == ERROR) return ERROR;

        fprintf(output, "\tPOP\tgr2\n");
        switch (opr) {
            case TPLUS:
                fprintf(output, "\tADDA\tgr1,gr2\n\tJOV\tEOVF\n");
                break;
            case TMINUS:
                fprintf(output, "\tSUBA\tgr2,gr1\n\tJOV\tEOVF\n\tLD\tgr1,gr2\n");
                break;
            case TOR:
                fprintf(output, "\tOR\tgr1,gr2\n");
                break;
            default:
                exit(EXIT_FAILURE);
        }

        switch (opr) {
            case TPLUS:
            case TMINUS:
                if (!(type1 == TPINT && type2 == TPINT)) return error("Operand type of '+' and '-' must be integer");
                type = TPINT;
                break;
            case TOR:
                if (!(type1 == TPBOOL && type2 == TPBOOL)) return error("Operand type of 'or' must be boolean");
                type = TPBOOL;
                break;
        }
    }
    return type;
}

/* 項 */
int term() {
    int type;
    if ((type = factor()) == ERROR) return (ERROR);
    while (token == TSTAR || token == TAND || token == TDIV) {
        int type1 = type, type2, opr = token;
        is_opr = 1;
        fprintf(output, "\tPUSH\t0,gr1\n");
        if (multiplicative_operator() == ERROR) return (ERROR);
        if ((type2 = factor()) == ERROR) return (ERROR);

        fprintf(output, "\tPOP\tgr2\n");
        switch (opr) {
            case TSTAR:
                fprintf(output, "\tMULA\tgr1,gr2\n");
                fprintf(output, "\tJOV\tEOVF\n");
                break;
            case TAND:
                fprintf(output, "\tAND\tgr1,gr2\n");
                break;
            case TDIV:
                fprintf(output, "\tDIVA\tgr2,gr1\n");
                fprintf(output, "\tJOV\tE0DIV\n");
                fprintf(output, "\tLD\tgr1,gr2\n");
                break;
            default:
                exit(EXIT_FAILURE);
        }

        switch (opr) {
            case TSTAR:
            case TDIV:
                if (!(type1 == TPINT && type2 == TPINT)) return error("Operand type of '*' and 'div' must be integer");
                type = TPINT;
                break;
            case TAND:
                if (!(type1 == TPBOOL && type2 == TPBOOL)) return error("Operand type of 'and' must be boolean");
                type = TPBOOL;
                break;
        }
    }
    return type;
}

/* 因子 */
int factor() {
    int type;
    switch (token) {
        case TNAME:
            if ((type = variable()) == ERROR) return ERROR;
            if (!(!is_opr && (ispara || iscallpara) && (token == TCOMMA || token == TRPAREN))) {
                fprintf(output, "\tLD\tgr1,0,gr1\n");
            }
            break;
        case TNUMBER:
        case TFALSE:
        case TTRUE:
        case TSTRING:
            if ((type = constant()) == ERROR) return ERROR;
            break;
        case TLPAREN:
            is_opr = 1;
            token = scan();
            if ((type = expression()) == ERROR) return ERROR;
            if (token != TRPAREN) return error("Symbol ')' is not found");
            token = scan();
            break;
        case TNOT:
            is_opr = 1;
            token = scan();
            if ((type = factor()) == ERROR) return ERROR;
            if (type != TPBOOL) return error("Operand type of 'not' must be boolean");
            fprintf(output, "\tCPA\tgr1,gr0\n");
            fprintf(output, "\tJZE\tL%04d\n", label);
            fprintf(output, "\tLD\tgr1,gr0\n");
            fprintf(output, "\tJUMP\tL%04d\n", label + 1);
            fprintf(output, "L%04d\n", label);
            fprintf(output, "\tLAD\tgr1,1\n");
            fprintf(output, "L%04d\n", label + 1);
            label = label + 2;
            break;
        case TINTEGER:
        case TBOOLEAN:
        case TCHAR:
            is_opr = 1;
            type = token_to_ttype(token);
            if (standard_type() == ERROR) return ERROR;
            if (token != TLPAREN) return error("Symbol '(' is not found");
            token = scan();
            int ex_type;
            if ((ex_type = expression()) == ERROR) return ERROR;
            if (is_standard_type(ex_type) == 0) return error("Type of cast expression must be standard type");

            switch (ex_type) {
                case TPINT:
                    switch (type) {
                        case TPINT:
                            break;
                        case TPCHAR:
                            fprintf(output, "\tLAD\tgr2,127\n\tAND\tgr1,gr2\n");
                            break;
                        case TPBOOL:
                            fprintf(output, "\tCPA\tgr1,gr0\n\tJZE\tL%04d\n\tLAD\tgr1,1\nL%04d\n", label, label);
                            label++;
                            break;
                        default:
                            exit(EXIT_FAILURE);
                    }
                    break;
                case TPCHAR:
                    switch (type) {
                        case TPINT:
                            break;
                        case TPCHAR:
                            break;
                        case TPBOOL:
                            fprintf(output, "\tCPA\tgr1,gr0\n\tJZE\tL%04d\n\tLAD\tgr1,1\nL%04d\n", label, label);
                            label++;
                            break;
                        default:
                            exit(EXIT_FAILURE);
                    }
                    break;
                case TPBOOL:
                    switch (type) {
                        case TPINT:
                            break;
                        case TPCHAR:
                            fprintf(output, "\tLAD\tgr2,127\n\tAND\tgr1,gr2\n");
                            break;
                        case TPBOOL:
                            break;
                        default:
                            exit(EXIT_FAILURE);
                    }
                    break;
                default:
                    exit(EXIT_FAILURE);
            }

            if (token != TRPAREN) return error("Symbol ')' is not found");
            token = scan();
            break;
        default:
            return error("Factor is not found");
    }
    return type;
}

/* 定数 */
int constant() {
    int type;
    if (token != TNUMBER && token != TFALSE && token != TTRUE && token != TSTRING)
        return error("Number or Keyword 'false' or 'true' or String is not found");
    switch (token) {
        case TNUMBER:
            fprintf(output, "\tLAD\tgr1,%d\n", num_attr);
            type = TPINT;
            break;
        case TFALSE:
        case TTRUE:
            if (token == TFALSE) {
                fprintf(output, "\tLAD\tgr1,0\n");
            } else {
                fprintf(output, "\tLAD\tgr1,1\n");
            }
            type = TPBOOL;
            break;
        case TSTRING:
            if (strlen(string_attr) > 1) return error("Length of char type str must be 1");
            fprintf(output, "\tLAD\tgr1,%d\n", string_attr[0]);
            type = TPCHAR;
            break;
    }
    token = scan();
    return type;
}

/* 乗法演算子 */
int multiplicative_operator() {
    if (token != TSTAR && token != TDIV && token != TAND)
        return error("Symbol '*' or Keyword 'div' or 'and' is not found");
    token = scan();
    return NORMAL;
}

/* 加法演算子 */
int additive_operator() {
    if (token != TPLUS && token != TMINUS && token != TOR)
        return error("Symbol '+' or '-' or Keyword 'or' is not found");
    token = scan();
    return NORMAL;
}

/* 関係演算子 */
int relational_operator() {
    if (token != TEQUAL && token != TNOTEQ &&
        token != TLE && token != TLEEQ &&
        token != TGR && token != TGREQ)
        return error("Symbol '=' or '<>' or '<' or '<=' or '>' or '>=' is not found");
    token = scan();
    return NORMAL;
}

/* 入力文 */
int input_statement() {
    int type;
    int is_readln = 0;

    if (token == TREADLN) is_readln = 1;
    if (token != TREAD && token != TREADLN)
        return error("Keyword 'read' or 'readln' is not found");
    token = scan();
    if (token == TLPAREN) {
        token = scan();
        if ((type = variable()) == ERROR) return ERROR;
        if (!(type == TPINT || type == TPCHAR))
            return error("Type of variable of input statement must be integer or char");

        if (type == TPINT) {
            fprintf(output, "\tCALL\tREADINT\n");
        } else {
            fprintf(output, "\tCALL\tREADCHAR\n");
        }

        while (token == TCOMMA) {
            token = scan();
            if ((type = variable()) == ERROR) return ERROR;
            if (!(type == TPINT || type == TPCHAR))
                return error("Type of variable of input statement must be integer or char");
        }
        if (token != TRPAREN)
            return error("Keyword ')' is not found");
        if (is_readln) fprintf(output, "\tCALL\tREADLINE\n");
        token = scan();
    }
    return NORMAL;
}

/* 出力文 */
int output_statement() {
    int is_writeln = 0;

    if (token == TWRITELN) is_writeln = 1;
    if (token != TWRITE && token != TWRITELN)
        return error("Keyword 'write' or 'writeln' is not found");
    token = scan();
    if (token == TLPAREN) {
        token = scan();
        if (output_format() == ERROR) return ERROR;
        while (token == TCOMMA) {
            token = scan();
            if (output_format() == ERROR) return ERROR;
        }
        if (token != TRPAREN)
            return error("Symbol ')' is not found");
        token = scan();
    }
    if (is_writeln) {
        fprintf(output, "\tCALL\tWRITELINE\n");
    }
    return NORMAL;
}

/* 出力指定 */
int output_format() {
    int type;
    if (token == TSTRING && strlen(string_attr) > 1) {
        fprintf(output, "\tLAD\tgr1,L%04d\n\tLD\tgr2,gr0\n\tCALL\tWRITESTR\n", label);
        register_strlabel(string_attr);
        token = scan();
    } else {
        /* 文字列の長さが1の場合は式から生成される定数の一つである"文字列"とする. */
        if ((type = expression()) == ERROR) return ERROR;
        if (!is_standard_type(type)) return error("Type of output format must be standard type");
        if (token == TCOLON) {
            token = scan();
            if (token != TNUMBER) return error("Number is not found");
            fprintf(output, "\tLAD\tgr2,%d\n", num_attr);
            token = scan();
        } else {
            fprintf(output, "\tLD\tgr2,gr0\n");
        }
        switch (type) {
            case TPINT:
                fprintf(output, "\tCALL\tWRITEINT\n");
                break;
            case TPCHAR:
                fprintf(output, "\tCALL\tWRITECHAR\n");
                break;
            case TPBOOL:
                fprintf(output, "\tCALL\tWRITEBOOL\n");
                break;
            default:
                exit(EXIT_FAILURE);
        }
    }
    return NORMAL;
}

/* 空文 */
int empty_statement() {
    return NORMAL;
}

int token_to_ttype(int token) {
    switch (token) {
        case TINTEGER:
            return TPINT;
        case TBOOLEAN:
            return TPBOOL;
        case TCHAR:
            return TPCHAR;
        case TARRAY:
            return TPARRAY;
    }
    return 0;
}

int is_standard_type(int type) {
    switch (type) {
        case TPINT:
        case TPBOOL:
        case TPCHAR:
            return 1;
        default:
            return 0;
    }
}

void reverse_show_parameter(struct PARA *p) {
    if (p != NULL) {
        reverse_show_parameter(p->nextparap);
        fprintf(output, "\tPOP\tgr1\n\tST\tgr1,$%s%%%s\n", p->name, procname);
    }
}