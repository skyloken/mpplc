//
// Created by ken on 2018/11/08.
//

#ifndef SOFTWARE5_EX2_PP_H
#define SOFTWARE5_EX2_PP_H

/* pp.h  */
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#define MAXSTRSIZE 1024

#define NORMAL 0
#define ERROR 1

#define TPNULL 2
#define TPINT 3
#define TPCHAR 4
#define TPBOOL 5
#define TPARRAY 6
#define TPPROC 7

#define NUMOFTTYPE 7


/* Token */
#define	TNAME		1	/* Name : Alphabet { Alphabet | Digit } */
#define	TPROGRAM	2	/* program : Keyword */
#define	TVAR		3	/* var : Keyword */
#define	TARRAY		4	/* array : Keyword */
#define	TOF		    5	/* of : Keyword */
#define	TBEGIN		6	/* begin : Keyword */
#define	TEND		7  	/* end : Keyword */
#define	TIF		    8  	/* if : Keyword */
#define	TTHEN		9	/* then : Keyword */
#define	TELSE		10	/* else : Keyword */
#define	TPROCEDURE	11	/* procedure : Keyword */
#define	TRETURN		12	/* return : Keyword */
#define	TCALL		13	/* call : Keyword */
#define	TWHILE		14	/* while : Keyword */
#define	TDO		    15 	/* do : Keyword */
#define	TNOT		16	/* not : Keyword */
#define	TOR		    17	/* or : Keyword */
#define	TDIV		18 	/* div : Keyword */
#define	TAND		19 	/* and : Keyword */
#define	TCHAR		20	/* char : Keyword */
#define	TINTEGER	21	/* integer : Keyword */
#define	TBOOLEAN	22 	/* boolean : Keyword */
#define	TREADLN		23	/* readln : Keyword */
#define	TWRITELN	24	/* writeln : Keyword */
#define	TTRUE		25	/* true : Keyword */
#define	TFALSE		26	/* false : Keyword */
#define	TNUMBER		27	/* unsigned integer */
#define	TSTRING		28	/* String */
#define	TPLUS		29	/* + : symbol */
#define	TMINUS		30 	/* - : symbol */
#define	TSTAR		31 	/* * : symbol */
#define	TEQUAL		32 	/* = : symbol */
#define	TNOTEQ		33 	/* <> : symbol */
#define	TLE		    34 	/* < : symbol */
#define	TLEEQ		35 	/* <= : symbol */
#define	TGR		    36	/* > : symbol */
#define	TGREQ		37	/* >= : symbol */
#define	TLPAREN		38 	/* ( : symbol */
#define	TRPAREN		39 	/* ) : symbol */
#define	TLSQPAREN	40	/* [ : symbol */
#define	TRSQPAREN	41 	/* ] : symbol */
#define	TASSIGN		42	/* := : symbol */
#define	TDOT		43 	/* . : symbol */
#define	TCOMMA		44	/* , : symbol */
#define	TCOLON		45	/* : : symbol */
#define	TSEMI		46	/* ; : symbol */
#define	TREAD		47	/* read : Keyword */
#define	TWRITE		48	/* write : Keyword */
#define	TBREAK		49	/* break : Keyword */

#define NUMOFTOKEN	49

/* cr.c */

#define KEYWORDSIZE	28

extern struct KEY {
    char *keyword;
    int keytoken;
} key[KEYWORDSIZE];

struct ID {
    char *name;
    char *procname; /* procedure name within which this name is defined */ /* NULL if global name */
    struct TYPE *itp;
    int ispara; /* 1:formal parameter, 0:else(variable) */
    int deflinenum;
    int is_output;
    struct LINE *irefp;
    struct PARA *iparap;
    struct ID *nextp;
};

struct TYPE {
    int ttype; /* TPINT TPCHAR TPBOOL TPARRAY TPPROC */
    int arraysize; /* size of array, if TPARRAY */
    int element_type;
};

struct LINE {
    int reflinenum;
    struct LINE *nextlinep;
};

struct PARA {
    int ttype;
    struct PARA *nextparap;
};

extern int error(char *mes);
extern void error_and_exit(char *mes, int linenum);

/* scan.c */
extern int init_scan(char *filename);
extern int scan(void);
extern int num_attr;
extern char string_attr[MAXSTRSIZE];
extern int get_linenum(void);
extern void end_scan(void);

/* parse.c */
extern int parse_program();
extern int token;
extern int ele_type;
extern struct PARA *f_para;

/* id-list.c */
extern void init_idtab();
extern void register_name(char *name, char *procname, int deflinenum, int ispara);
extern void register_type_to_name(int ttype, int array_size, int element_type);
extern int register_reflinenum(char *name, char *procname, int reflinenum);
extern void register_procedure_parameter(char *procname);
extern void print_idtab();

/* string of each token */
extern char *tokenstr[NUMOFTOKEN + 1];
extern char *ttypestr[NUMOFTTYPE + 1];

#endif //SOFTWARE5_EX2_PP_H
