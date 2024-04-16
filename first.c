#define _CRT_SECURE_NO_WARNINGS
#include <stdio.h>
#include <string.h>
#include <ctype.h>

#define Max_symbols 100 // Number of terminals and nonterminals in the given grammar must be smaller than this number.
#define Max_size_rule_table 200  //  actual number of Rrles must be smaller than this number.
#define Max_string_of_RHS 20	 // the maximum number of symbols on the right side of rules

typedef struct ssym {
	int kind; // 0 if terminal; 1 if nonterminal; -1 if dummy symbol indicating the end mark.
	int no; // the unique index of a symbol.
	char str[10]; // string of the symbol
} sym;  // definition symbols 

// Rules are represented as follows
typedef struct orule {  // rule �� ���� ����.
	sym leftside;
	sym rightside[Max_string_of_RHS];
	int  rleng;  // RHS �� �ɺ��� ��.  0 is given if righthand side is epsilon.
} onerule;

typedef struct {
	int r, X, i, Yi;
} type_recompute;

int Max_terminal;
int Max_nonterminal;
sym Nonterminals_list[Max_symbols];
sym Terminals_list[Max_symbols];

int Max_rules;  // ���� �� ������ ������.
onerule Rules[Max_size_rule_table];	// �� ���� ������.

int First_table[Max_symbols][Max_symbols]; // actual region:  Max_nonterminal  X (MaxTerminals+2)
int done_first[Max_symbols];	// ��ܸ���ȣ�� first ��� �Ϸ� �÷���

type_recompute recompute_list[500];    // recompute_list point list
int num_recompute; // number of recompute_list points in recompute_list.
type_recompute a_recompute;

int ch_stack[100] = { -1, }; //call history stack. -1 �� ���� ��. �ʱ�ȭ ���ص� �������.
int top_stack = -1;

int lookUp_nonterminal(char* str) {
	int i;
	for (i = 0; i < Max_nonterminal; i++)
		if (strcmp(str, Nonterminals_list[i].str) == 0)
			return i;
	return -1;
}

int lookUp_terminal(char* str) {
	int i;
	for (i = 0; i < Max_terminal; i++)
		if (strcmp(str, Terminals_list[i].str) == 0)
			return i;
	return -1;
}

void read_grammar(char* fileName) {
	FILE* fp;
	char line[500]; // line buffer
	char symstr[10];
	char* ret;
	int i, k, n_sym, n_rule, i_leftSymbol, i_rightSymbol, i_right, synkind;
	fp = fopen(fileName, "r");
	if (!fp) {
		printf("File open error of grammar file.\n");
		getchar(); // make program pause here.
	}

	ret = fgets(line, 500, fp); // ignore line 1
	ret = fgets(line, 500, fp); // ignore line 2
	ret = fgets(line, 500, fp); // read nonterminals line.
	i = 0; n_sym = 0;
	do { // read nonterminals.
		while (line[i] == ' ' || line[i] == '\t') i++; // skip spaces.
		if (line[i] == '\n') break;
		k = 0;
		while (line[i] != ' ' && line[i] != '\t' && line[i] != '\n')
		{
			symstr[k] = line[i]; i++; k++;
		}
		symstr[k] = '\0'; // a nonterminal string is finished.
		strcpy(Nonterminals_list[n_sym].str, symstr);
		Nonterminals_list[n_sym].kind = 1; // nonterminal.
		Nonterminals_list[n_sym].no = n_sym;
		n_sym++;
	} while (1);
	Max_nonterminal = n_sym;

	i = 0; n_sym = 0;
	ret = fgets(line, 500, fp); // read terminals line.
	do { // read terminals.
		while (line[i] == ' ' || line[i] == '\t') i++; // skip spaces.
		if (line[i] == '\n') break;
		k = 0;
		while (line[i] != ' ' && line[i] != '\t' && line[i] != '\n')
		{
			symstr[k] = line[i]; i++; k++;
		}
		symstr[k] = '\0'; // a terminal string is finished.
		strcpy(Terminals_list[n_sym].str, symstr);
		Terminals_list[n_sym].kind = 0; // terminal.
		Terminals_list[n_sym].no = n_sym;
		n_sym++;
	} while (1);
	Max_terminal = n_sym;

	ret = fgets(line, 500, fp); // ignore one line.
	n_rule = 0;
	do { // reading Rules.
		ret = fgets(line, 500, fp);
		if (!ret)
			break; // no characters were read. So reading Rules is terminated.

		// if the line inputed has only white spaces, we should skip this line.
		// this is determined as follows: length==0; or first char is not an alphabet.
		i = 0;
		if (strlen(line) < 1)
			continue;
		else {
			while (line[i] == ' ' || line[i] == '\t') i++; // skip spaces.
			if (!isalpha(line[i]))
				continue;
		}

		// take off left symbol of a rule.
		while (line[i] == ' ' || line[i] == '\t') i++; // skip spaces.
		k = 0;
		while (line[i] != ' ' && line[i] != '\t' && line[i] != '\n')
		{
			symstr[k] = line[i]; i++; k++;
		}
		symstr[k] = '\0'; // a nonterminal string is finished.
		i_leftSymbol = lookUp_nonterminal(symstr);
		if (i_leftSymbol < 0) {
			printf("Wrong left symbol of a rule.\n");
			getchar();
		}

		// left symbol is stored of the rule.
		Rules[n_rule].leftside.kind = 1; Rules[n_rule].leftside.no = i_leftSymbol;
		strcpy(Rules[n_rule].leftside.str, symstr);

		// By three lines below, we move to first char of first sym of RHS.
		while (line[i] != '>') i++;
		i++;
		while (line[i] == ' ' || line[i] == '\t') i++;

		// Collect the symbols of the RHS of the rule.
		i_right = 0;
		do { // reading symbols of RHS
			k = 0;
			while (i < strlen(line) && (line[i] != ' '
				&& line[i] != '\t' && line[i] != '\n'))
			{
				symstr[k] = line[i]; i++; k++;
			}
			symstr[k] = '\0';
			if (strcmp(symstr, "epsilon") == 0) { // this is epsilon rule.
				Rules[n_rule].rleng = 0; // declare that this rule is an epsilon rule.
				break;
			}

			if (isupper(symstr[0])) { // this is nonterminal
				synkind = 1;
				i_rightSymbol = lookUp_nonterminal(symstr);
			}
			else { // this is terminal
				synkind = 0;
				i_rightSymbol = lookUp_terminal(symstr);
			}

			if (i_rightSymbol < -1) {
				printf("Wrong right symbol of a rule.\n");
				getchar();
			}

			Rules[n_rule].rightside[i_right].kind = synkind;
			Rules[n_rule].rightside[i_right].no = i_rightSymbol;
			strcpy(Rules[n_rule].rightside[i_right].str, symstr);

			i_right++;

			while (line[i] == ' ' || line[i] == '\t') i++;
			if (i >= strlen(line) || line[i] == '\n') // i >= strlen(line) is needed in case of eof just after the last line.
				break; // finish reading  righthand symbols.
		} while (1); // loop of reading symbols of RHS.

		Rules[n_rule].rleng = i_right;
		n_rule++;
	} while (1); // loop of reading Rules.

	Max_rules = n_rule;
	printf("Total number of Rules = %d\n", Max_rules);
} // read_grammar.

// Is nonterminal Y in recompute list?
int is_nonterminal_in_recompute_list(int Y) {
	int i;
	for (i = 0; i < num_recompute; i++) {
		if (recompute_list[i].X == Y)
			return 1;
	}
	return 0;
}  // end of is_nonterminal_in_recompute_list

// is a nonterminal in ch_stack?
int nonterminal_is_in_stack(int Y) {
	int i;
	if (top_stack >= 0) {
		for (i = 0; i <= top_stack; i++)
			if (ch_stack[i] == Y)
				return 1;
	}
	return 0;
} // end of nonterminal_is_in_stack

//  first computation of one nonterminal with capability of dealing with case-2.
void first(sym X) {              // assume X is a nonterminal.
	int i, r, rleng, ie;
	sym Yi;

	top_stack++;
	ch_stack[top_stack] = X.no;  // push to the stack.
	printf("first(%s) starts. \n", X.str);

	for (r = 0; r < Max_rules; r++) {
		if (Rules[r].leftside.no != X.no)
			continue; // skip this rule since left side is not X.
		rleng = Rules[r].rleng;
		if (rleng == 0) {
			First_table[X.no][Max_terminal] = 1; // �ԽǷ��� first ��  �߰�.
			printf("epsilon is added to first of %s.\n", X.str);
			continue;  // ���� ��� ����.
		}

		printf("Begin to use rule %d: %s -> ", r, Rules[r].leftside.str);
		for (ie = 0; ie < rleng; ie++) printf(" %s", Rules[r].rightside[ie].str);
		printf("\n");

		for (i = 0; i < rleng; i++) {
			printf("Use the rule\'s right side symbol: %s\n", Rules[r].rightside[i].str);
			Yi = Rules[r].rightside[i];	// Yi �� ���� �������� ��ġ i �� �ɺ�
			if (Yi.kind == 0) {	// Yi is terminal
				First_table[X.no][Yi.no] = 1;	// Yi�� X�� first �� �߰�.
				printf("%s is added to first of %s.\n", Yi.str, X.str);
				break;	// exit this loop to go to next rule.
			}
			// Now, Yi is nonterminal.
			if (X.no == Yi.no) {	// case-1 �߻���.
				printf("Case 1  has occurred: rule: %d, at RHS position %d, Left symbol: %s\n", r, i, X.str);
				if (First_table[X.no][Max_terminal] == 1) {
					printf("first of %s has epsilon. So symbols after %s in RHS is processed.\n", X.str, X.str);
					continue;  // epsilon �� first of X �� �����Ƿ� Yi �������� ó���ϰ� ��.
				}
				else
					printf("first of %s does not have epsilon. So move to next rule.\n", X.str);
				break; // epsilon �� first of X �� �����Ƿ� Yi�� ������ �����ϰ� ���� ��� ����.
			}

			if (done_first[Yi.no] == 1) {	// Yi �� first ����� �̹� ������.
				if (is_nonterminal_in_recompute_list(Yi.no)) {	// Yi �� �½ɺ��� �������� �����ϸ�,
					a_recompute.r = r; a_recompute.X = X.no; a_recompute.i = i; a_recompute.Yi = Yi.no;
					recompute_list[num_recompute] = a_recompute;	// ������ [r,X,i,Yi] �� �ִ´�.
					num_recompute++;
					printf("������ �߰�:[%d,%s,%d,%s]\n\n", r, Nonterminals_list[X.no].str, i, Nonterminals_list[Yi.no].str);
				}
			}
			else {	// done of Yi == 0�̴�. �� Yi �� first ����� ���� ������.
				if (nonterminal_is_in_stack(Yi.no)) {	// Yi �� ch_stack �� �ִٸ�
					printf("Case_3 occurrs: calling first(%s) cannot be done since it is in stack.\n", Yi.str);
					a_recompute.r = r; a_recompute.X = X.no; a_recompute.i = i; a_recompute.Yi = Yi.no;
					recompute_list[num_recompute] = a_recompute;	// ������ [r,X,i,Yi] �� �ִ´�.
					num_recompute++;
					printf("������ �߰�:[%d,%s,%d,%s]\n\n", r, Nonterminals_list[X.no].str, i, Nonterminals_list[Yi.no].str);
				}
				else {	// Yi �� ch_stack �� ����.
					printf("\nCall first(%s) in first(%s).\n", Yi.str, X.str);
					first(Yi);	// Yi �� first �� ����Ͽ� First_table �� �����.
					printf("Return from first(%s) to first(%s). \n\n", Yi.str, X.str);
					if (is_nonterminal_in_recompute_list(Yi.no)) {	// Yi�� �½ɺ��� �������� �ִٸ�,
						a_recompute.r = r; a_recompute.X = X.no; a_recompute.i = i; a_recompute.Yi = Yi.no;
						recompute_list[num_recompute] = a_recompute;	// ������ [r,X,i,Yi] �� �ִ´�.
						num_recompute++;
						printf("������ �߰�:[%d,%s,%d,%s]\n\n", r, Nonterminals_list[X.no].str, i, Nonterminals_list[Yi.no].str);
					}
				}  // else
			} // else
			// Yi �� first �� ���� �߿� epsilon �� �ƴ� �͵��� first_X �� �־��ش�. 
			int n;
			for (n = 0; n < Max_terminal; n++) {
				if (First_table[Yi.no][n] == 1) {
					if (First_table[X.no][n] == 0) {
						First_table[X.no][n] = 1;
						printf("%s in first of %s is added to first of %s.\n", Terminals_list[n].str, Yi.str, X.str);
					}
				}
			}
			// epsilon�� Yi �� first �� ���ٸ� ���� ��� ����. �ִٸ� Yi �� ���� �� �ɺ��� ����. 
			if (First_table[Yi.no][Max_terminal] == 0) {
				printf("Move to next rule since first of %s does not have epsilon.\n", Yi.str);
				break;
			}
		} // for (i=0
		// break �� �������� ����� �´ٸ� i != rleng �̴�. �׷��� �ʴٸ� i==rleng �̰�,
		// �� ���� r ��Ģ�� �������� ��� �ɺ��� first �� epsilon�� ������. ��� X �� ������ �Ѵ�.
		if (i == rleng) {
			First_table[X.no][Max_terminal] = 1;  // X �� first �� epsilon�� ������ �Ѵ�.
			printf("epsilon is added to first of %s.\n", X.str);
		}
	} // for (r=0

	done_first[X.no] = 1;  // X �� done (first ���Ϸ�)�� 1 �� �Ѵ�.

	// pop stack.
	ch_stack[top_stack] = -1;  // dummy ���� �ִ´�. ��� �� ���� ���ص� ��!
	top_stack--;  // �� ���� ������ pop �� �ϴ� ����.
	printf("first(%s) ends. \n", X.str);
} // end of first

// alpha is an arbitrary string of terminals or nonterminals. 
// A dummy symbol with kind = -1 must be placed at the end as the end marker.
// length: number of symbols in alpha.
void first_of_string(sym alpha[], int length, int first_result[]) {
	int i, k;
	sym Yi;
	for (i = 0; i < Max_terminal + 1; i++)
		first_result[i] = 0; // initialize the first result of alpha

	// Let alpha be Y0 Y1 ... Y(length-1)

	for (i = 0; i < length; i++) {
		Yi = alpha[i];
		if (Yi.kind == 0) {  // Yi is terminal
			first_result[Yi.no] = 1;
			break;
		}
		else {  // ���� ���� Yi �� ��ܸ���ȣ�̴�.
			for (k = 0; k < Max_terminal; k++)	 // copy first of Yi to first of alpha
				if (First_table[Yi.no][k] == 1) first_result[k] = 1;
			if (First_table[Yi.no][Max_terminal] == 0)
				break; // first of Yi does not have epsilon. So forget remaining part.	
		}
	} // for 
	if (i == length)
		first_result[Max_terminal] = 1;  // epsilon �� �ִ´�.
} // end of function first_of_string

// ��� ��ܸ���ȣ�� first �� ���Ѵ�. (������ ó���� �����Ͽ� case-2���� ó����.)
void first_all() {
	int i, j, r, m, A, k, n, Xno;
	sym X, Y;

	// ���� ��� ��ܸ���ȣ���� first �� ���Ͽ� First_table �� ����Ѵ�.
	for (i = 0; i < Max_nonterminal; i++) {
		X = Nonterminals_list[i];
		if (done_first[i] == 0) { // ���� �� ��ܸ���ȣ�� ó������ ����.
			top_stack = -1; // ������ ��� ��. (�� ���� ����� ���ص� ��.)
			first(X);
		}
		if (top_stack != -1) {
			printf("Logic error. stack top should be -1.\n");
			getchar();  // �ϴ� ���߰� ��.
		}
	} // for

	// ������ ó��
	int change_occurred;
	type_recompute recom;

	printf("\nNumber of recomputation points = %d\n", num_recompute);
	while (1) {
		// ���� �ѹ� �� ��, ��� ���������� ó���Ѵ�.
		printf("\nLoop of processing recompuation list starts.\n");
		change_occurred = 0;
		for (m = 0; m < num_recompute; m++) {
			recom = recompute_list[m];
			r = recom.r; Xno = recom.X; i = recom.i; A = recom.Yi;
			k = Rules[r].rleng;
			printf("\nrecomputation point to process: [%d, %s, %d, %s]\n", r, Nonterminals_list[Xno].str, i, Nonterminals_list[A].str);
			for (j = i; j < k; j++) {
				Y = Rules[r].rightside[j];
				if (Y.kind == 0) {  // �ܸ���ȣ�̸�,
					if (First_table[Xno][Y.no] == 0) {
						change_occurred = 1;
						printf("%s is added to first of %s in recomputing\n", Y.str, Nonterminals_list[Xno].str);
					}
					First_table[Xno][Y.no] = 1;
					break;  // ��Ģ r �� ó�� �����ϰ�, ���� ���������� ����.
				}
				// ���� ���� Y�� ��ܸ���ȣ��. Y �� first �� ��� X �� first�� ����Ѵ�(�ԽǷ� ����).
				for (n = 0; n < Max_terminal; n++) {
					if (First_table[Y.no][n] == 1) {
						if (First_table[Xno][n] == 0) {
							change_occurred = 1;
							printf("%s in first of %s is added to first of %s in recomputing\n", Terminals_list[n].str, Y.str, Nonterminals_list[Xno].str);
						}
						First_table[Xno][n] = 1;
					}  // if
				}  // for(n=0
				if (First_table[Y.no][Max_terminal] == 0)	// Y �� first �� �ԽǷ��� ���ٸ�,
					break;  // Y ������ ����.
			} // for (j=i
			if (j == k) {  // �̰��� ���̸�, ���� ������ ��� �ɺ��� �ԽǷ��� ����.
				if (First_table[Xno][Max_terminal] == 0) {
					change_occurred = 1;
					printf("epsilon is added to first of %s in recomputing by rule %d\n", Nonterminals_list[Xno].str, r);
				}
				First_table[Xno][Max_terminal] = 1;  // �ԽǷ��� �־� ��.
			} // if
		} // for (m=0
		if (change_occurred == 0)
			break;	// ������ ó������ ���� ��ȭ�� ������.
	} // while
	printf("\nComputing first for all nonterminal symbols ends.\n");
} // end of first_all


int main()
{
	int i, j, res;
	char line[200];
	char grammar_file_name[100];
	sym a_nonterminal = { 1, 0 };

	printf("���� ������ �̸��� �����ÿ�>");
	fgets(line, 199, stdin);
	res = sscanf(line, "%s", grammar_file_name);
	if (res != 1) {
		printf("���� �̸� �Է� ����.");
		return 0;
	}

	read_grammar(grammar_file_name);

	// initialze First table.
	for (i = 0; i < Max_nonterminal; i++) {
		for (j = 0; j < (Max_terminal + 1); j++) {   // 0 ~ Max_terminal including epsilon.
			First_table[i][j] = 0;
		}
		done_first[i] = 0;
	}

	num_recompute = 0;  // ������ ����Ʈ�� ��� ���´�.

	// ��� ��ܸ���ȣ�� ���Ͽ� first �� ����Ѵ�.
	first_all();

	// Print first of all nonterminals.
	printf("\nFirst table of all nonterminals:\n");
	for (i = 0; i < Max_nonterminal; i++) {
		printf("First(%s): ", Nonterminals_list[i].str);
		for (j = 0; j < Max_terminal; j++) {
			if (First_table[i][j] == 1)
				printf("%s  ", Terminals_list[j].str);
		}
		if (First_table[i][Max_terminal] == 1)
			printf(" eps"); // epsilon
		printf("\n");
	}

	printf("\nProgram terminates.\n");
} // main.