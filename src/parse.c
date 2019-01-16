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
int ispara;
int is_local;
int is_call;
int ele_type;
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

/* プログラム */
int parse_program() {
    if (token != TPROGRAM) return (error("Keyword 'program' is not found"));
    token = scan();
    if (token != TNAME) return (error("Program name is not found"));
    token = scan();
    if (token != TSEMI) return (error("Semicolon is not found"));
    token = scan();
    if (block() == ERROR) return (ERROR);
    if (token != TDOT) return (error("Period is not found at the end of program"));
    token = scan();
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
    if (compound_statement() == ERROR) return ERROR;
    return NORMAL;
}

/* 変数宣言部 */
int variable_declaration() {
    ispara = 0;
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
    if (compound_statement() == ERROR) return ERROR;
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

    switch (token) {
        case TNAME:
            if (assignment_statement() == ERROR) return ERROR;
            break;
        case TIF:
            if (condition_statement() == ERROR) return ERROR;
            break;
        case TWHILE:
            if (iteration_statement() == ERROR) return ERROR;
            break;
        case TBREAK:
            if (exit_statement() == ERROR) return ERROR;
            break;
        case TCALL:
            if (call_statement() == ERROR) return ERROR;
            break;
        case TRETURN:
            if (return_statement() == ERROR) return ERROR;
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
    if (token != TIF) return error("Keyword 'if' is not found");
    token = scan();
    if ((type = expression()) == ERROR) return ERROR;
    if(type != TPBOOL) return error("Conditional expression of condition statement must be boolean");
    if (token != TTHEN) return error("Keyword 'then' is not found");
    token = scan();
    if (statement() == ERROR) return ERROR;
    if (token == TELSE) {
        token = scan();
        if (statement() == ERROR) return ERROR;
    }
    return NORMAL;
}

/* 繰り返し文 */
int iteration_statement() {
    int type;
    if (token != TWHILE) return error("Keyword 'while' is not found");
    token = scan();
    if ((type = expression()) == ERROR) return ERROR;
    if(type != TPBOOL) return error("Conditional expression of iteration statement must be boolean");
    if (token != TDO) return error("Keyword 'do' is not found");
    token = scan();
    if (statement() == ERROR) return ERROR;
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
    if (token != TCALL) return error("Keyword 'call' is not found");
    token = scan();
    if (procedure_name() == ERROR) return ERROR;
    if (token == TLPAREN) {
        no_arg = 0;
        token = scan();
        if (expressions() == ERROR) return ERROR;
        if (token != TRPAREN) return error("Symbol ')' is not found");
        token = scan();
    }
    if (no_arg && f_para != NULL) return error("Number of procedure arguments do not match");
    is_call = 0;
    return NORMAL;
}

/* 式の並び */
int expressions() {
    int type;
    if ((type = expression()) == ERROR) return ERROR;
    if (f_para == NULL) return error("Number of procedure arguments do not match");
    if (type != f_para->ttype) return error("Procedure argument types do not match");
    while (token == TCOMMA) {
        if (f_para->nextparap == NULL) return error("Number of procedure arguments do not match");
        f_para = f_para->nextparap;
        token = scan();
        if ((type = expression()) == ERROR) return ERROR;
        if (type != f_para->ttype) return error("Procedure argument types do not match");
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
    if ((type2 = expression()) == ERROR) return ERROR;
    if (type1 != type2) return error("Left part and expression type must be same type");
    if (!(is_standard_type(type1) && is_standard_type(type2)))
        return error("Left part and expression type must be standard type");
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
        if ((sub_type = expression()) == ERROR) return ERROR;
        if (sub_type != TPINT) return error("Array subscript type must be integer");
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
        if (relational_operator() == ERROR) return ERROR;
        if ((type2 = simple_expression()) == ERROR) return ERROR;
        if (type1 != type2) return error("Operands of relational operator must be same type");
        if (!(is_standard_type(type1) && is_standard_type(type2)))
            return error("Operand of relational operator must be standard type");
        type = TPBOOL;
    }
    return type;
}

/* 単純式 */
int simple_expression() {
    int is_right_ope = 0, type;
    if (token == TPLUS || token == TMINUS) {
        is_right_ope = 1;
        token = scan();
    }
    if ((type = term()) == ERROR) return ERROR;
    if (is_right_ope && type != TPINT) return error("When there is '+' or '-', Type of left term must be integer");
    while (token == TPLUS || token == TMINUS || token == TOR) {
        int type1 = type, type2, ope = token;
        if (additive_operator() == ERROR) return ERROR;
        if ((type2 = term()) == ERROR) return ERROR;
        switch (ope) {
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
        int type1 = type, type2, ope = token;
        if (multiplicative_operator() == ERROR) return (ERROR);
        if ((type2 = factor()) == ERROR) return (ERROR);
        switch (ope) {
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
            break;
        case TNUMBER:
        case TFALSE:
        case TTRUE:
        case TSTRING:
            if ((type = constant()) == ERROR) return ERROR;
            break;
        case TLPAREN:
            token = scan();
            if ((type = expression()) == ERROR) return ERROR;
            if (token != TRPAREN) return error("Symbol ')' is not found");
            token = scan();
            break;
        case TNOT:
            token = scan();
            if ((type = factor()) == ERROR) return ERROR;
            if (type != TPBOOL) return error("Operand type of 'not' must be boolean");
            break;
        case TINTEGER:
        case TBOOLEAN:
        case TCHAR:
            type = token_to_ttype(token);
            if (standard_type() == ERROR) return ERROR;
            if (token != TLPAREN) return error("Symbol '(' is not found");
            token = scan();
            int ex_type;
            if ((ex_type = expression()) == ERROR) return ERROR;
            if (is_standard_type(ex_type) == 0) return error("Type of cast expression must be standard type");
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
            type = TPINT;
            break;
        case TFALSE:
        case TTRUE:
            type = TPBOOL;
            break;
        case TSTRING:
            type = TPCHAR;
            if (strlen(string_attr) > 1) return error("Length of char type string must be 1");
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
    if (token != TREAD && token != TREADLN)
        return error("Keyword 'read' or 'readln' is not found");
    token = scan();
    if (token == TLPAREN) {
        token = scan();
        if ((type = variable()) == ERROR) return ERROR;
        if (!(type == TPINT || type == TPCHAR))
            return error("Type of variable of input statement must be integer or char");
        while (token == TCOMMA) {
            token = scan();
            if ((type = variable()) == ERROR) return ERROR;
            if (!(type == TPINT || type == TPCHAR))
                return error("Type of variable of input statement must be integer or char");
        }
        if (token != TRPAREN)
            return error("Keyword ')' is not found");
        token = scan();
    }
    return NORMAL;
}

/* 出力文 */
int output_statement() {
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
    return NORMAL;
}

/* 出力指定 */
int output_format() {
    int type;
    if (token == TSTRING) {
        if (strlen(string_attr) > 1) {
            token = scan();
        } else {
            /* 文字列の長さが1の場合は式から生成される定数の一つである"文字列"とする. */
            if ((type = expression()) == ERROR) return ERROR;
            if (!is_standard_type(type)) return error("Type of output format must be standard type");
            if (token == TCOLON) {
                token = scan();
                if (token != TNUMBER) return error("Number is not found");
                token = scan();
            }
        }
    } else {
        if ((type = expression()) == ERROR) return ERROR;
        if (!is_standard_type(type)) return error("Type of output format must be standard type");
        if (token == TCOLON) {
            token = scan();
            if (token != TNUMBER) return error("Number is not found");
            token = scan();
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