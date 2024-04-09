#define _CRT_SECURE_NO_WARNINGS
//
// lexical analysis 를 수행하는 프로그램: C 언어 소스프로그램의 토큰들을 대부분 인식함.
//   - comment 처리,
//   - string constant, character constant 토큰도 분석한다.

#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <math.h>

#define UNK 53     // 인식할 수 없는 토큰을 나타내는 토큰 번호. 최종 토큰 번호로 ?? 을 고쳐주어야 함.
#define EOF_TOK 52 // token index of EOF token

// 문자열상수의 토큰번호: 54;  문자상수의 토큰번호: 55.

typedef struct tkt { // 하나의 토큰을 나타내는 구조체.
	int index;  // index : 토큰의 종류를 나타냄, 즉 토큰 번호를 말함.
	char typ[16];
	char sub_kind[3];  // relop 토큰인 경우 다시 구분하는데 이용됨.
						 // NUM 토큰의 양의 정수인지("in") 실수인지를("do") 구분.
	int sub_data; // ID 토큰의 심볼테이블엔트리 번호, 양의 정수 값을 저장함.
	double rnum;  // 실수인 경우 
	char str[100]; // 토큰을 구성하는 스트링.  스트링상수나 문자상수에서 이용함.
	char string_constant[100]; // 문자열상수의 내용
	char char_constant; // 문자상수의 내용
} tokentype;

typedef char oneword[50];

// keyword 테이블  
// token index of "if" is 28. token index of other keywords are computed using this info.
char keywords[31][50] =
{ "if",    "else",  "while",  "do",   "for",    "include", "define", "typedef","struct","int",
   "char", "float", "double", "void", "return", "case",    "then",    "true",   "false" 
,	"auto", "break" , "continue", "default", "enum", "exturn", "goto", "long", "static", "switch"
, "union", "unsigned"};
int total_keywords = 31; // 나중에 수정 요망.

//  symbol 테이블 
typedef struct sym { oneword idstr; int attribute; } sym_ent;  // 심볼테이블의 한 엔트리의 구조.
sym_ent symtbl[500];
int total_ids = 0; // 심볼테이블에서 다음에 넣을 엔트리 번호

FILE* fp, * ofp;

int iswhitespace(char c) {
	if (c == ' ' || c == '\n' || c == '\t')
		return 1;
	else
		return 0;
}

int lookup_keyword_tbl(char* str) {
	int i;
	for (i = 0; i < total_keywords; i++)
		if (strcmp(keywords[i], str) == 0)
			return i;
	return -1;

}

int lookup_symtbl(char* str) {
	int i;
	for (i = 0; i < total_ids; i++)
		if (strcmp(symtbl[i].idstr, str) == 0)
			return i;
	return -1;
}
//*
//-------------------------------------------------------------
tokentype lexan() {
	int state = 0;
	char c; // 다음 글자를 읽어 옴.
	char buf[110]; // 토큰의 전체 스트링을 임시 저장함.
	char str_constant[100] = "";  // 문자열상수의 내용을 일시 저장함
	char ch_constant; // 문자상수의 내용을 일시 저장함.

	int bp = 0; // bp is buffer pointer(다음 넣을 위치).
	int upper_n; // number 토큰에서 소숫점 위 부분 즉 정수 부분을 저장함.
	double fraction; // number 토큰에서 소숫점 아래 부분을 저장함.
	tokentype token;
	int idx, FCNT, sign, Enum, i;
	strcpy(token.typ, " ");
	while (1) {
		switch (state) {
		case 0: // 초기 상태. 각 토큰의 첫 글자에 따라서 작업 수행 및 다음 상태 결정함.
			c = fgetc(fp);  // fgetc can be called even if fp is after the end of file.
							  // calling it again still returns EOF(-1) w/o invoking error.
			if (iswhitespace(c)) state = 0;  // this is white space.
			else if (isalpha(c) || c == '_') { buf[bp] = c; bp++; buf[bp] = '\0'; state = 28; }
			else if (isdigit(c)) { buf[bp] = c; bp++; buf[bp] = '\0'; upper_n = c - '0'; state = 1; }
			else if (c == '<') state = 2;
			else if (c == '>') state = 32;
			else if (c == '=') state = 35;
			else if (c == '!') state = 38;
			else if (c == '+') state = 3;
			else if (c == '-') state = 4;
			else if (c == '*') state = 52;
			else if (c == '/') state = 8;
			else if (c == '\\') state = 53;
			else if (c == '%') state = 54;
			else if (c == '.') state = 55;
			else if (c == ',') state = 56;
			else if (c == '(') state = 57;
			else if (c == ')') state = 58;
			else if (c == '{') state = 59;
			else if (c == '}') state = 60;
			else if (c == '[') state = 61;
			else if (c == ']') state = 62;
			else if (c == ':') state = 63;
			else if (c == ';') state = 64;
			else if (c == '\"') { state = 65; buf[bp] = c; bp++; buf[bp] = '\0'; }
			else if (c == '\'') state = 66;
			else if (c == '#') state = 67;
			else if (c == '|') state = 68;
			else if (c == '&') state = 5;
			else if (c == EOF) state = 71;
			else {
				token.index = UNK; // 인식할 수 없는 토큰임을 나타냄.
				return token;
			}
			break;
		case 1: // NUM 토큰의 소수점 위 숫자열을 받아 들이는 상태.
			c = fgetc(fp);
			if (isdigit(c)) { buf[bp] = c; bp++; buf[bp] = '\0'; upper_n = 10 * upper_n + c - '0'; state = 1; }
			else if (c == '.') { buf[bp] = c; bp++; buf[bp] = '\0'; fraction = 0; FCNT = 0; state = 9; } // 소수점을 나왔으므로 실수를 처리하는 상태로 감.
			else if (c == 'E') { buf[bp] = c; bp++; buf[bp] = '\0'; fraction = 0; state = 16; }  // E 가 있는 exponent 처리부로 감.
			else state = 14;
			break;
		case 2: // 각괄호글자 < 가 나온 후의 처리를 담당하는 상태.
			c = fgetc(fp);
			if (c == '=') state = 30;
			else state = 31;
			break;
		case 3: // 각괄호글자 + 가 나온 후의 처리를 담당하는 상태.
			c = fgetc(fp);
			if (c == '=') state = 45;
			else if (c == '+') state = 47;
			else state = 46;
			break;
		case 4: // 각괄호글자 - 가 나온 후의 처리를 담당하는 상태.
			c = fgetc(fp);
			if (c == '-') state = 48;
			else if (c == '=') state = 49;
			else if (c == '>') state = 51;
			else state = 50;
			break;
		case 5: // 각괄호글자 + 가 나온 후의 처리를 담당하는 상태.
			c = fgetc(fp);
			if (c == '&') state = 6;
			else state = 7;
			break;
		case 6:
			token.index = 2; strcpy(token.sub_kind, "");		 strcpy(token.str, "&&");
			return token;
		case 7: // 토큰 & 를 리턴해주는 상태.
			ungetc(c, fp);
			token.index = 13;
			return token;
		case 8:
			c = fgetc(fp);
			if (c == '/') state = 74;
			else if (c == '*') state = 75;
			else if (c == EOF) state = 71;
			else state = 72;
			break;
		case 9: // 실수의 소수점 이하를 받아 들이는 상태
			c = fgetc(fp);
			if (isdigit(c)) {
				buf[bp] = c; bp++; buf[bp] = '\0';
				FCNT++; fraction = fraction + (c - '0') / pow(10.0, FCNT); state = 23;
			}
			else if (c == 'E') { buf[bp] = c; bp++; buf[bp] = '\0'; state = 16; }
			else if (c == EOF)  state = 26;
			else if (iswhitespace(c)) state = 24;
			else state = 25;// goto unknown token state
			break;
		case 14: // 양의 정수 토큰을 리턴하는 상태.
			ungetc(c, fp);
			token.index = 1; strcpy(token.sub_kind, "in"); // 양의 정수를 나타냄.
			token.sub_data = upper_n;
			strcpy(token.str, buf);
			strcpy(token.typ, "Number");
			return token;
		case 16:
			c = fgetc(fp);
			if (c == '+') { buf[bp] = c; bp++; buf[bp] = '\0'; sign = 1; state = 17; }
			else if (c == '-') { buf[bp] = c; bp++; buf[bp] = '\0'; sign = -1; state = 17; }
			else if (isdigit(c)) { buf[bp] = c; bp++; buf[bp] = '\0'; sign = 1; Enum = c - '0'; state = 18; }
			else  state = 25; // error! 		 
			break;
		case 17:
			c = fgetc(fp);
			if (isdigit(c)) { buf[bp] = c; bp++; buf[bp] = '\0'; Enum = c - '0'; state = 18; }
			else state = 25; // error!
			break;
		case 18:
			c = fgetc(fp);
			if (isdigit(c)) { buf[bp] = c; bp++;  buf[bp] = '\0'; Enum = Enum * 10 + c - '0'; state = 18; }
			else state = 19;
			break;
		case 19:
			ungetc(c, fp);
			token.index = 1; strcpy(token.sub_kind, "do"); // 실수를 나타냄.
			token.rnum = (upper_n + fraction) * pow(10.0, sign * Enum);
			strcpy(token.str, buf);
			strcpy(token.typ, "Number");
			return token;

		case 23:
			c = fgetc(fp);
			if (isdigit(c)) {
				buf[bp] = c; bp++; buf[bp] = '\0';
				FCNT++; fraction = fraction + (c - '0') / pow(10.0, FCNT); state = 23;
			}
			else if (c == 'E') { buf[bp] = c; bp++; buf[bp] = '\0'; state = 16; }
			else state = 24;
			break;
		case 24:
			ungetc(c, fp);
			token.index = 1; strcpy(token.sub_kind, "do"); // 실수를 나타냄.
			token.rnum = upper_n + fraction;
			strcpy(token.str, buf);
			strcpy(token.typ, "Number");
			return token;
		case 25:
			ungetc(c, fp);
			token.index = 53; // unknown token
			return token;
		case 26:  // do not call ungetc.
			token.index = 1; strcpy(token.sub_kind, "do"); // 실수를 나타냄.
			token.rnum = upper_n + fraction;
			strcpy(token.str, buf);
			strcpy(token.typ, "Number");
			return token;
		case 28:
			c = fgetc(fp);
			if (isalpha(c) || isdigit(c) || c == '_') { buf[bp] = c; bp++; buf[bp] = '\0'; state = 28; }
			else	 state = 29;
			break;
		case 29: // id 나 keyword 
			ungetc(c, fp);
			strcpy(token.str, buf);
			idx = lookup_keyword_tbl(buf); // -1 if not exist.
			if (idx >= 0 && idx <= 18) { token.index = 31 + idx; strcpy(token.typ, "Keyword"); return token; }  // Note: first keyword has token index 30.
			// reaches here if it is not a keyword.
			else if (idx > 18) { token.index = 37 + idx; strcpy(token.typ, "Keyword"); return token; }
			idx = lookup_symtbl(buf); // -1 if not exist.
			if (idx >= 0) { token.index = 0; token.sub_data = idx; strcpy(token.typ, "Identifier"); return token; }
			// reaches here if it is not in symbol table.
			strcpy(symtbl[total_ids].idstr, buf); total_ids++;
			token.index = 0; // ID 토큰임을 나타냄.
			token.sub_data = total_ids - 1; // 이 ID 가 들어 있는 심볼테이블 엔트리 번호.
			strcpy(token.typ, "Identifier");
			return token;
		case 30:
			token.index = 2; strcpy(token.sub_kind, "LE"); strcpy(token.str, "<=");
			strcpy(token.typ, "ROP");
			return token;
		case 31:
			ungetc(c, fp);
			token.index = 2; strcpy(token.sub_kind, "LT"); strcpy(token.str, "<");
			strcpy(token.typ, "ROP");
			return token;
		case 32:
			c = fgetc(fp);
			if (c == '=') state = 33;
			else state = 34;
			break;
		case 33:
			token.index = 2; strcpy(token.sub_kind, "GE");	strcpy(token.str, ">=");
			strcpy(token.typ, "ROP");
			return token;
		case 34:
			ungetc(c, fp);
			token.index = 2; strcpy(token.sub_kind, "GT"); strcpy(token.str, ">");
			strcpy(token.typ, "ROP");
			return token;
		case 35: // 글자 = 가 나온 후의 처리를 담당하는 상태.
			c = fgetc(fp);
			if (c == '=') state = 36;
			else state = 37;
			break;
		case 36: // 토큰 == 에 대한 처리를 수행하는 상태.
			token.index = 2; strcpy(token.sub_kind, "EQ");	strcpy(token.str, "==");
			strcpy(token.typ, "ROP");
			return token;
		case 37: // 토큰 > 에 대한 처리를 수행하는 상태.
			ungetc(c, fp);
			token.index = 8; strcpy(token.str, "=");
			strcpy(token.typ, "Symbol");
			return token;
		case 38:
			c = fgetc(fp);
			if (c == '=') state = 39;
			else state = 40;
			break;
		case 39:
			token.index = 2; strcpy(token.sub_kind, "NE");	 strcpy(token.str, "!=");
			strcpy(token.typ, "ROP");
			return token;
		case 40:
			ungetc(c, fp);
			token.index = 10;  strcpy(token.str, "!"); // NOT		
			strcpy(token.typ, "ROP");
			return token;
		case 45:
			token.index = 16; 		 strcpy(token.str, "+=");
			return token;
		case 46:
			ungetc(c, fp);
			token.index = 3;  strcpy(token.str, "+");
			strcpy(token.typ, "Symbol");
			return token;
		case 47:
			token.index = 14;		 strcpy(token.str, "++");
			strcpy(token.typ, "Symbol");
			return token;
		case 48:
			token.index = 15;		 strcpy(token.str, "--");
			strcpy(token.typ, "Symbol");
			return token;
		case 49:
			ungetc(c, fp);
			token.index = 17;  strcpy(token.str, "-=");
			strcpy(token.typ, "Symbol");
			return token;
		case 50:
			ungetc(c, fp);
			token.index = 4;		 strcpy(token.str, "-");
			strcpy(token.typ, "Symbol");
			return token;
		case 51:
			token.index = 9;		 strcpy(token.str, "->");
			strcpy(token.typ, "Symbol");
			return token;
		case 52:
			token.index = 5;		 strcpy(token.str, "*");
			strcpy(token.typ, "Symbol");
			return token;
		case 53:
			token.index = 30;		 strcpy(token.str, "\\");
			strcpy(token.typ, "Symbol");
			return token;
		case 54:
			token.index = 7;		 strcpy(token.str, "%");
			strcpy(token.typ, "Symbol");
			return token;
		case 55:
			token.index = 11;		 strcpy(token.str, ".");
			strcpy(token.typ, "Symbol");
			return token;
		case 56:
			token.index = 12;		 strcpy(token.str, ",");
			strcpy(token.typ, "Symbol");
			return token;
		case 57:
			token.index = 18;		 strcpy(token.str, "(");
			strcpy(token.typ, "Symbol");
			return token;
		case 58:
			token.index = 19;		 strcpy(token.str, ")");
			strcpy(token.typ, "Symbol");
			return token;
		case 59:
			token.index = 20;		 strcpy(token.str, "{");
			strcpy(token.typ, "Symbol");
			return token;
		case 60:
			token.index = 21;		 strcpy(token.str, "}");
			strcpy(token.typ, "Symbol");
			return token;
		case 61:
			token.index = 22;		 strcpy(token.str, "[");
			strcpy(token.typ, "Symbol");
			return token;
		case 62:
			token.index = 23;		 strcpy(token.str, "]");
			strcpy(token.typ, "Symbol");
			return token;
		case 63:
			token.index = 24;		 strcpy(token.str, ":");
			strcpy(token.typ, "Symbol");
			return token;
		case 64:
			token.index = 25;		 strcpy(token.str, ";");
			strcpy(token.typ, "Symbol");
			return token;
		case 65:  // 큰 따옴표로 문자열상수가 시작됨.
			c = fgetc(fp);
			if (c == '\\') { // back-slash 글자가 나오면.
				buf[bp] = c; bp++; buf[bp] = '\0'; state = 78;
			}
			else if (c == '\"') {  // 큰 따옴표가 나오면.
				buf[bp] = c; bp++; buf[bp] = '\0'; state = 79;
			}
			else if (c == EOF) // end of file 이면.
				state = 80;
			else { // other 글자이면.
				buf[bp] = c; bp++; buf[bp] = '\0'; i = strlen(str_constant);
				str_constant[i] = c;  str_constant[i + 1] = '\0';
				state = 65;
			}
			break;
		case 66:// 작은 따옴표로 문자상수가 시작됨.
			buf[bp] = c; bp++; buf[bp] = '\0'; // 문자 상수의 첫 글자인 '.
			c = fgetc(fp);
			if (c == '\\') { //  back-slash 글자가 나오면.
				buf[bp] = c; bp++; buf[bp] = '\0'; state = 82;
			}
			else { // other 글자
				buf[bp] = c; bp++; buf[bp] = '\0';
				ch_constant = c;
				state = 83;
			}
			break;
		case 67:
			token.index = 28;		 strcpy(token.str, "#");
			strcpy(token.typ, "Symbol");
			return token;
		case 68:
			c = fgetc(fp);
			if (c == '|') 	state = 69;
			else 	 state = 70;
			break;
		case 69:
			token.index = 2;	strcpy(token.sub_kind, "OR");	 strcpy(token.str, "||");
			strcpy(token.typ, "Symbol");
			return token;
		case 70:
			ungetc(c, fp);
			token.index = 29;	 strcpy(token.str, "|");
			strcpy(token.typ, "Symbol");
			return token;
		case 71:
			token.index = EOF_TOK; strcpy(token.str, "EOF");
			strcpy(token.typ, "EOF");
			return token;
		case 72:
			ungetc(c, fp);
			token.index = 6; strcpy(token.str, "/");
			return token;
		case 73:
			token.index = 6; strcpy(token.str, "/");
			strcpy(token.typ, "Symbol");
			return token;
		case 74:
			c = fgetc(fp);
			if (c == '\n') state = 0;
			else if (c == EOF) state = 71;
			else state = 74;
			break;
		case 75:
			c = fgetc(fp);
			if (c == '*') state = 76;
			else if (c == EOF) state = 71;
			else state = 75;
			break;
		case 76:
			c = fgetc(fp);
			if (c == '/') state = 0;
			else if (c == EOF) state = 71;
			else state = 75;
			break;
		case 78: c = fgetc(fp);
			if (c == 'n') {
				i = strlen(str_constant); str_constant[i] = 'n';  // 여기서 10은 '\n' 글자의 아스키 값임.
				str_constant[i + 1] = '\0';
			}
			else if (c == 't') {
				i = strlen(str_constant); str_constant[i] = 't';  // 여기서 9은 '\t' 글자의 아스키 값임.
				str_constant[i + 1] = '\0';
			}
			else if (c == '\"') { // 큰 따옴표가 나오면.
				i = strlen(str_constant); str_constant[i] = c;  str_constant[i + 1] = '\0';
			}
			else if (c == EOF)
				state = 80;
			else { // other 처리.
				i = strlen(str_constant); str_constant[i] = '\\';  str_constant[i + 1] = c; str_constant[i + 2] = '\0';
			}

			if (c == EOF)
				state = 80;  // c 가 EOF 이라면 다음 state 는 80 이고, 아니면 65 이어야 함.
			else {
				buf[bp] = c; bp++; buf[bp] = '\0';
				state = 65;
			}
			break;
		case 79: // 문자열상수가 완성됨.
			token.index = 54; strcpy(token.str, buf); strcpy(token.string_constant, str_constant);
			strcpy(token.typ, "Str_consant");
			return token;
		case 80: // UNK(nowen) 토큰
			token.index = UNK; strcpy(token.str, buf); strcpy(token.typ, "Unknown");
			return token;
		case 81:
		case 82: c = fgetc(fp);
			buf[bp] = c; bp++; buf[bp] = '\0';
			if (c == '0') {
				ch_constant = 0; state = 83;
			} // 널문자
			else if (c == 'n') {
				ch_constant = 10; state = 83;
			} // new line 글자
			else if (c == 't') {
				ch_constant = 9; state = 83;
			} // tab 글자
			else if (c == '\\') {
				bp--;
				buf[bp] = '\0';
				ch_constant = 92; state = 83;
			}
			else
				state = 80; // Other 글자가 오면 UNK 가 온 것으로 간주.
			break;
		case 83: c = fgetc(fp);
			if (c == '\'') {
				buf[bp] = c; bp++; buf[bp] = '\0'; state = 84;
			}
			else if (c == EOF)
				state = 80;
			else {
				buf[bp] = c; bp++; buf[bp] = '\0'; state = 80;
			}
			break;
		case 84: // 문자상수가 완성됨.
			token.index = 55; strcpy(token.str, buf); token.char_constant = ch_constant;
			strcpy(token.typ, "Char_consant");
			return token;
			break;
		default: printf("Unrecognizable token! Stop generating tokens.\n");
			token.index = UNK; strcpy(token.str, "UNK");
			return token;
		} // switch
	} //while
}

//--------------------------------------
void print_token(tokentype a_tok, FILE* ofp) {
	fprintf(ofp, "%s\tToken_idx: %d,  %s ", a_tok.str, a_tok.index, a_tok.typ); // 토큰 종류 출력 (종류는 스트링으로 대체함)
	printf("%s\tToken_idx: %d,  %s ", a_tok.str, a_tok.index, a_tok.typ);

	if (a_tok.index == 1) { // this is number token.
		if (strcmp(a_tok.sub_kind, "in") == 0) {
			fprintf(ofp, "   integer  Val= %d", a_tok.sub_data); // 정수토큰임.
			printf("   integer  Val= %d", a_tok.sub_data);
		}
		else if (strcmp(a_tok.sub_kind, "do") == 0) {
			fprintf(ofp, "   double  Val= %10.7f", a_tok.rnum); // 실수 토큰임.
			printf("   double  Val= %10.7f", a_tok.rnum);
		}
	}
	else if (a_tok.index == 0) {
		fprintf(ofp, "  Symtbl_idx = %5d", a_tok.sub_data); // id 의 심볼테이블 엔트리.
		printf("  Symtbl_idx = %5d", a_tok.sub_data);
	}
	else;
	fprintf(ofp, "\n");
	printf("\n");

	fflush(ofp);
}
//*/
//-------------------------------------------------------------
void main(void) {
	tokentype a_tok;
	char source_file[30] = "sourcefile.txt";
	char output_file[30] = "outputfile.txt";
	char c;
	int i;

	fp = fopen("sourcefile.txt", "r"); // 입력 소스 화일
	if (!fp) { printf("file open error of file = %s", source_file); return; }
	ofp = fopen("outputfile.txt", "w"); // 출력 화일
	if (!ofp) { printf("file open error of file = %s", output_file); return; }

	while (1) {
		a_tok = lexan();
		if (a_tok.index == EOF_TOK) // EOF 토큰이면,
			break; // there will be no more token. so exit.
		else if (a_tok.index == UNK)
			break; // terminate token generation since wrong token has appeared.
		else
			print_token(a_tok, ofp);
	}
	fclose(fp);
	fclose(ofp);
	printf("\nProgram ends with success.\n\n");
	return;
} // main

