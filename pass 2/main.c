#define _CRT_SECURE_NO_WARNINGS
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <ctype.h>

#define SIZE 30

struct node {
    char* name;
    int address;
    struct node* next;
};

struct Node {
    struct instruction_line* instruction;
    int PC;
    struct Node* next;
    struct Node* previous;
};

struct instruction_line {
    char* symbol;
    char* opCode;
    char* operand_1;
    char* operand_2;
}instruct;

const int cap = 32768;                // memory capacity
struct node table[SIZE];              // hash table
struct Node head;
int program_counter[2] = {0x0,0x0};   // Program counter
int line = 0;                         // line number


void   Tokenize( char* s, struct instruction_line* instruct);
int    is_directive( char* s );
int    hash( char* name );
int    add_symbol(char* symbol, int ma);
void   print_table();
void print_list();
int    calc_address(const struct instruction_line *i);
struct node* find_symbol(char* sym);
char*  opcode_gen(char* n);
int add_Node(struct instruction_line* i, int pc);

int main( int argc, char* argv[] )
{
    //initialize hash table
    for( int i = 0; i < SIZE; i++ ) {
        table[i].name = NULL;
        table[i].address = 0;
        table[i].next = NULL;
    }
    //initialize the head Node
    head.PC = 0;
    head.instruction = NULL;
    head.next     = NULL;
    head.previous = NULL;


/*
    // check correct # of args
    if(argc != 2){
        printf("ERROR: Invalid use of argument\n");
        return 1;
    }
*/
    FILE* inputFile;
    inputFile = fopen( "text.txt", "r" );

    if( !inputFile ) {
        printf( "ERROR: Could not open %s for reading\n", argv[1] );
        return 1;
    }

    int flag= 0;

    /*
    * read file line by line
    */
    char str[1024];
    while( fgets( str, 1024, inputFile ) ) {
        line++;
        //printf("---------------------> %s\n", str);
        Tokenize( str, &instruct);
        //printf("-------%s\t%s\t%s\t%s\t%d\n", instruct.symbol, instruct.opCode, instruct.operand_1, instruct.operand_2, program_counter[0]);
        if( instruct.opCode != NULL ) {

            // if opcode equals to "START", set PC to its operand
            if( !flag ) {
                if( !program_counter[0] && !( strcmp( "START", instruct.opCode ) ) ) {

                    if(instruct.operand_1 == NULL){
                        printf( "ERROR: (line %d) \"START\" directive must have an address\n",line );
                        exit(1);
                    }
                    if(strspn(instruct.operand_1, "0123456789abcdefABCDEF") != strlen(instruct.operand_1)){
                        printf("ERROR: (line %d) %s is not a valid hex\n", line, instruct.operand_1);
                        exit(1);
                    }
                    flag = 1;

                    program_counter[0] = strtol( instruct.operand_1, NULL, 16 );
                    program_counter[1] = program_counter[0];

                    if(program_counter[1] > cap){
                        printf("ERROR: (line %d) the memory limit exceeded\n", line);
                        exit(1);
                    }
                    add_Node(&instruct,program_counter[1]);
                    add_symbol(instruct.symbol, program_counter[1]);
                    continue;
                } else {
                    printf("ERROR: (line %d) source file must have a START directive\n", line);
                    exit(1);
                }
            }
            // catch duplicate START
            if(!strcmp(instruct.opCode,"START")){
                printf("ERROR: (line %d) duplicate \"START\" directive\n", line);
                exit(1);
            }
            // if symbol, add to symbol table
            if( instruct.symbol != NULL ) {

                program_counter[0] += calc_address(&instruct);

                if(program_counter[1] > cap){
                    printf("ERROR: (line %d) the memory limit exceeded\n", line);
                    exit(1);
                }

                // print error if duplicate symbols found
                add_Node(&instruct,program_counter[1]);
                if(add_symbol(instruct.symbol, program_counter[1])){
                    printf( "ERROR: (line %d) duplicate symbol name \"%s\"\n",line, instruct.symbol );
                    exit(1);
                }
            } else {
                add_Node(&instruct,program_counter[1]);
                program_counter[0] += 3;
            }
            program_counter[1] = program_counter[0];
            if(program_counter[1] > cap){
                    printf("ERROR: (line %d) the memory limit exceeded\n", line);
                    exit(1);
            }
            if(!strcmp(instruct.opCode, "END")) flag++;
        }

    }
    if(flag != 2) printf("ERROR: \"END\" directive is missing form the source file\n");

    //print_table();
   // print_list(&head);
    //generate_object_file();

    fclose( inputFile );

    //START OF PASS II
    inputFile = fopen( "text.txt", "r" );
    if( !inputFile ) {
        printf( "ERROR: Could not open %s for reading\n", argv[1] );
        return 1;
    }
    line = 0;

    // Read first input line
    if( fgets( str, 1024, inputFile ) ) {
        line++;
        Tokenize( str, &instruct);
    }else {printf("End of file\n");exit(1);}

    // read input until 'START'
    while(strcmp(instruct.opCode, "START")){
        if( fgets( str, 1024, inputFile ) ) {
            line++;
            Tokenize( str, &instruct);
        }else {
            printf("End of file\n");
            exit(1);
        }
    }

    char *token = NULL;


    // create a file for writing
    FILE* fp = fopen("o.txt", "w");
    fprintf(fp, "H");
    fp = fopen("o.txt", "a");
    fprintf(fp, " ");

    // Header: program name
    fprintf(fp, instruct.symbol);
    for(int i = strlen(instruct.symbol); i < 6; i++) fprintf(fp, " ");

    // Header: starting address
    for(int i = strlen(instruct.operand_1); i < 6; i++) fprintf(fp, "0");
    fprintf(fp, instruct.operand_1);

    program_counter[0] = strtol( instruct.operand_1, NULL, 16 );

    char pc_buffer[5];
    itoa(program_counter[1] - program_counter[0],pc_buffer , 16 );

    // Header: length of object program
    for(int i = strlen(pc_buffer); i < 6; i++) fprintf(fp, "0");
    for(int i = 0; i < strlen(pc_buffer); i++){
        pc_buffer[i] = toupper(pc_buffer[i]);
    }
    fprintf(fp, pc_buffer);
    /*
    **
    **
    ** END OF Header*/


    // object code
    char oc_buffer[5] = {0};
    char str_buffer[1024] = {0};

    // initialize first T record
    fprintf(fp, "\n");
    program_counter[1]=program_counter[0];

    // Read first input line
    if( fgets( str, 1024, inputFile ) ) {
        line++;
        Tokenize( str, &instruct);
    }else {printf("End of file\n");exit(1);}




    /* START OF T TEXT RECORD
    **
    **
    */
    flag = 0;
    while(!flag){
        if(is_directive(instruct.opCode) == 0){
            if(strcmp(instruct.opCode, "BYTE")==0){

                char *token = NULL;
                pc_buffer[0] = '\0';
                if(instruct.operand_1[0] == 'C'){
                    token = strtok( instruct.operand_1, "'" );
                    token = strtok( NULL, "'" );

                    int temp_buffer_size = (strlen(token) * 2)+1;
                    char temp_buffer[temp_buffer_size];
                    temp_buffer[0] = '\0';

                    for(int i = 0; i < strlen(token); i++){
                        int t = (int)token[i];

                        if(t <= 15){strcat(temp_buffer, "0");}

                        itoa(t, pc_buffer, 16);
                        strcat(temp_buffer, pc_buffer);
                        pc_buffer[0] = '\0';
                    }

                    strcat(str_buffer, temp_buffer);
                    program_counter[1] += strlen(token);


                }else if(instruct.operand_1[0] == 'X'){
                    token = strtok( instruct.operand_1, "'" );
                    token = strtok( NULL, "'" );
                    strcat(str_buffer, token);
                    program_counter[1] += strlen(token)/2;
                }

            } else if (strcmp(instruct.opCode, "WORD")==0){

                int t = atoi(instruct.operand_1);
                pc_buffer[0] = '\0';
                itoa(t, pc_buffer, 16);
                for(int i = 6-strlen(pc_buffer); i>0 ; i--){
                    strcat(str_buffer, "0");
                }
                strcat(str_buffer, pc_buffer);
                program_counter[1] += 3;

            } else if(strcmp(instruct.opCode, "RESW")==0){
                int t = strtol(instruct.operand_1, NULL, 10);

                program_counter[1] = program_counter[1] + (t * 3);
            }else if(strcmp(instruct.opCode, "RESB")==0){
                int t = (int)atoi(instruct.operand_1);

                program_counter[1] = program_counter[1] + t;

            }

        // if instruction is valid
        }else if(opcode_gen(instruct.opCode)){

            strcpy(oc_buffer,opcode_gen(instruct.opCode) );

            // if operand 1 is empty
            if(!instruct.operand_1){
                oc_buffer[2] = '0';
                oc_buffer[3] = '0';
                oc_buffer[4] = '0';
                oc_buffer[5] = '0';
           }else{
                if(find_symbol(instruct.operand_1) != NULL){
                    pc_buffer[0]='\0';
                    itoa(find_symbol(instruct.operand_1)->address, pc_buffer , 16 );
                } else {
                    printf("ERROR: pass 2 (line %d) \"%s\" is not present in the symbol table\n", line,instruct.operand_1);
                    exit(1);
                }

                // indexed addressing
                if(instruct.operand_2){

                    int int_temp = pc_buffer[0] - '0';
                    int_temp += 8;
                    if(int_temp < 16){
                        itoa(int_temp, pc_buffer, 16);
                    }else{
                        printf("ERROR: (line %d) index addressing ");
                        exit(1);
                    }
                }
                strcat(&oc_buffer[2], pc_buffer);
            }

            // add object code to buffer
            strcat(str_buffer, oc_buffer);
            program_counter[1] += 3;

            //printf("%s\n", str_buffer);

        }else{
            printf("ERROR: (line %d) \"%s\" is not a valid SIC instruction", line, instruct.opCode);
            exit(1);
        }

        // CHECKPOINT
        if((program_counter[1] - program_counter[0]) <= 25){
            // Read next input line
            if( fgets( str, 1024, inputFile ) ) {
                line++;
                Tokenize( str, &instruct);
            }else {printf("End of file\n");exit(1);}

            if(strcmp(instruct.opCode, "END") != 0){
                continue;
            }else flag = 1;
        }

        pc_buffer[0] = '\0';
        // Text: write 'T' to the object file
        fprintf(fp, "T");
        // Text: add address to object file
        itoa(program_counter[0],pc_buffer , 16 );
        for(int i = strlen(pc_buffer); i < 6; i++) fprintf(fp, "0");
        for(int i = 0; i < strlen(pc_buffer); i++){
            pc_buffer[i] = toupper(pc_buffer[i]);
        }
        fprintf(fp, pc_buffer);

        pc_buffer[0] = '\0';
        // Text: append the length to object file
        itoa( program_counter[1]-program_counter[0],pc_buffer , 16 );
        for(int i = strlen(pc_buffer); i < 2; i++) fprintf(fp, "0");
        for(int i = 0; i < strlen(pc_buffer); i++){
            pc_buffer[i] = toupper(pc_buffer[i]);
        }
        fprintf(fp, pc_buffer);

        // Text: append string buffer to object file
        for(int i = 0; i < strlen(str_buffer); i++){
            str_buffer[i] = toupper(str_buffer[i]);
        }
        fprintf(fp, str_buffer);
        str_buffer[0] = '\0';

        program_counter[0]=program_counter[1];
        fprintf(fp, "\n");

        if(flag)break;
        // Read next input line
        if( fgets( str, 1024, inputFile ) ) {
            line++;
            Tokenize( str, &instruct);
        }else {printf("End of file\n");exit(1);}

        if(strcmp(instruct.opCode, "END") == 0)
            flag = 1;
    }
    /*
    **
    **
    * END OF TEXT RECORD */


    oc_buffer[0] = '\0';
    int t = find_symbol(instruct.operand_1)->address;
     itoa(t, oc_buffer, 16);

    //End: finalize
    fprintf(fp, "E");
    for(int i = strlen(&oc_buffer[0]); i < 6; i++) fprintf(fp, "0");
    for(int i = 0; i < strlen(oc_buffer); i++){
        oc_buffer[i] = toupper(oc_buffer[i]);
    }
    fprintf(fp, oc_buffer);
    fclose(fp);
    fclose(inputFile);

    return 0;
}

/*****************************************************
**
** tokenize a string & copy to instruction line
**
*****************************************************/
void Tokenize( char* s, struct instruction_line* instruct) {

    // reset struct
    instruct->symbol = NULL;
    instruct->opCode = NULL;
    instruct->operand_1 = NULL;
    instruct->operand_2 = NULL;

    // if comment (string starts with #) ignore
    if( s[0] == '#' ) return;
    if(( s[0] == '\r' )||( s[0] == '\n' ) ) {
        printf("ERROR: (line %d) source file cannot contain blank line\n", line);
        exit(1);
    }
    char* tokens = strtok( s, " \t\n" );
    if( s[0] != '\t' ) {

        // check to see if line starts with char A-Z (if so it's a symbol)
        if( ( s[0] >= 65 ) && ( s[0] <= 90 ) ) {
            instruct->symbol = tokens;
            if( !is_directive( tokens ) ) {
                printf( "ERORR: (line %d) symbol name cannot be defined with a name that matches an assembler directive\n", line );
                exit( 1 );
            }
            else if( strlen( tokens ) > 6 ) {
                printf( "ERROR: (line %d) symbol cannot be longer than 6 characters\n", line );
                exit( 1 );
            } else if(strspn(tokens, "$!=+-()@ ") != 0) {
                printf( "ERROR: (line %d) symbol cannot contain paces, $, !, =, +, - , (,  ), or @\n", line );
                exit( 1 );
            }
            tokens = strtok( NULL, " \t\n" );
        }
        else {
            printf( "ERROR: (line %d) symbol must start with an uppercase character\n", line );
            exit( 1 );
        }
    }
    instruct->opCode = tokens;
    tokens = strtok( NULL, " \n\t" );
    if( tokens == NULL ) return;

    // add ',' to strtok delimiter for two operand instructions
    if( strchr( tokens, ',' ) != NULL ) {
        char* temp = strtok( tokens, "," );
        instruct->operand_1 = temp;
        temp = strtok( NULL, "\n\t " );
        instruct->operand_2 = temp;
    }
    else
        instruct->operand_1 = tokens;
}
/******************************************
**
** returns 0 if 's' matches a directive
**
******************************************/
int is_directive( char* s ) {
    if( strcmp( s, "START" ) == 0) return 0;
    if( strcmp( s, "END" )  == 0 ) return 0;
    if( strcmp( s, "BYTE" ) == 0 ) return 0;
    if( strcmp( s, "WORD" ) == 0 ) return 0;
    if( strcmp( s, "RESB" ) == 0 ) return 0;
    if( strcmp( s, "RESW" ) == 0 ) return 0;
    if( strcmp( s, "RESR" ) == 0 ) return 0;
    if( strcmp( s, "EXPORTS" ) == 0 ) return 0;
    return 1;
}

/********************************************
**
** hash function
**
*********************************************/
int hash( char* name ) {
    int hash_val = 0;
    int l = strlen( name );
    for( int i = 0; i < l; i++ ) {
        hash_val += name[i];
        hash_val *= 13; // prime#
        hash_val %= SIZE;
    }
    return hash_val;
}
/********************************************
**
** add symbols to symbol table
**
********************************************/
int add_symbol(char *symbol, int ma) {

    // allocate memory for a single node
    struct node *temp = (struct node*)malloc(sizeof(struct node));
    temp->name = (char*)malloc(sizeof(symbol));
    temp->next = NULL;

    // add name & address to the node
    strcpy(temp->name, symbol);
    temp->address = ma;

    // hash using symbol name
    int x = hash(temp->name);

    //output
    //printf("%s\t%X\n", temp->name, temp->address);

    if (table[x].name != NULL) {
        struct node* _head = &table[x];
        // look for duplicate in linked list
        while (1) {

            // return 1 if duplicate found
            if (strcmp(_head->name, temp->name) == 0) {
                free(temp->name);
                free(temp);
                return 1;
            }
            // move head forward
            if (_head->next) {
                _head = _head->next;
                continue;
            } else {
            // add temp as new head and insert into table
                _head->next = temp;
                break;
            }
            return 0;
        }
    }else table[x] = *temp;
    return 0;
}

/********************************************
**
** print hash table
**
*********************************************/
void print_table(){
    for(int index =0; index< SIZE; index++ ){
        printf("\n[%.2d] ", index);
        if (table[index].name != NULL) {
            struct node* _head = &table[index];
            printf(" (%s, %X) -> ", _head->name, _head->address);
            while (_head->next != NULL) {
               _head = _head->next;
               printf(" (%s, %X) ->", _head->name, _head->address);
            }
            printf("\n");
        }else
         printf("\n");
    }
}
/********************************************
**
** return address of current symbol in decimal
**
*********************************************/
int calc_address(const struct instruction_line *i){
    if(strcmp(i->opCode, "RESW")== 0) return (int)atoi(i->operand_1) * 3;
    if(strcmp(i->opCode, "RESB")== 0) return (int)atoi(i->operand_1);
    if(strcmp(i->opCode, "WORD")== 0){

       // set limit for integer constant
       if(((int)atoi(i->operand_1) >= -8388607) && ((int)atoi(i->operand_1)<= 8388608))
            return 3;
        else{
            printf("ERROR: (line %d) integer %d exceeds WORD size\n", line, (int)atoi(i->operand_1));
            exit(1);
        }
    }
    if(strcmp(i->opCode, "BYTE")== 0){
        if(i->operand_1[0] == 'C'){
            char *token = strtok( i->operand_1, "'" );
            token = strtok( NULL, "'" );
            return strlen(token);
        }else if(i->operand_1[0] == 'X'){
            char *token = strtok( i->operand_1, "'" );
            token = strtok( NULL, "'" );

            if((strlen(token) == 0) || (strlen(token)%2 != 0)) {
                printf("ERROR: (line %d) \"%s\" is not a valid hex\n", line, token);
                exit(1);
            }
           if(strspn(token, "0123456789abcdefABCDEF") != strlen(token)){
                printf("ERROR: (line %d) \"%s\" is not a valid hex\n", line, token);
                exit(1);
            }
            return strlen(token)/2;
        }else {
            printf("ERROR: (line %d) directive \"BYTE\" does not have a proper operand\n", line);
            exit(1);
        }
    }
    return 3;
}

/********************************************
**
** return node with symbol name *sym in the table.
** return null if symbol doesn't exist
**
*********************************************/
struct node* find_symbol(char* sym){
    int i = hash(sym);
    if (table[i].name != NULL){

        // create a head pointer
        struct node* _head = &table[i];
        if(!strcmp(sym, _head->name)) return _head;
        while(_head->next != NULL){
            _head = _head->next;
            if(!strcmp(sym, _head->name)) return _head;
        }
    }
    return NULL;
}

/***************************************************
**
** return corresponding opcode for SIC instruction
**
***************************************************/
char* opcode_gen(char* n){
    if(!strcmp(n, "ADD" )) return "18";    if(!strcmp(n, "AND" )) return "40";
    if(!strcmp(n, "COMP")) return "28";    if(!strcmp(n, "DIV" )) return "24";
    if(!strcmp(n, "J"   )) return "3C";    if(!strcmp(n, "JEQ" )) return "30";
    if(!strcmp(n, "JGT" )) return "34";    if(!strcmp(n, "JLT" )) return "38";
    if(!strcmp(n, "JSUB")) return "48";    if(!strcmp(n, "LDA" )) return "00";
    if(!strcmp(n, "LDCH")) return "50";    if(!strcmp(n, "LDL" )) return "08";
    if(!strcmp(n, "LDX" )) return "04";    if(!strcmp(n, "MUL" )) return "20";
    if(!strcmp(n, "OR"  )) return "44";    if(!strcmp(n, "RD"  )) return "D8";
    if(!strcmp(n, "RSUB")) return "4C";    if(!strcmp(n, "STA" )) return "0C";
    if(!strcmp(n, "STCH")) return "54";    if(!strcmp(n, "STL" )) return "14";
    if(!strcmp(n, "STSW")) return "E8";    if(!strcmp(n, "STX" )) return "10";
    if(!strcmp(n, "SUB" )) return "1C";    if(!strcmp(n, "TD"  )) return "E0";
    if(!strcmp(n, "TIX" )) return "2C";    if(!strcmp(n, "WD"  )) return "DC";
    return NULL;

}
/**********************************************************
**
** add Node of Instruction_Line to the LinkedList
**
**********************************************************/
int add_Node(struct instruction_line* i, int pc){

    // Allocate memory for new instruction
    struct instruction_line new_instruc = {0};

    char* symbol_temp;
    if(i->symbol != NULL){
        symbol_temp = (char*)malloc(strlen(i->symbol) * sizeof(char));
        strcpy(symbol_temp, i->symbol);
    }
    else {symbol_temp = NULL;}

    char* opCode_temp;
    if(i->opCode != NULL){
        opCode_temp = (char*)malloc(strlen(i->opCode) * sizeof(char));
        strcpy(opCode_temp, i->opCode);
    }
    else {opCode_temp = NULL;}

    char* operand_1_temp;
    if(i->operand_1 != NULL){
        operand_1_temp = (char*)malloc(strlen(i->operand_1) * sizeof(char));
        strcpy(operand_1_temp, i->operand_1);
    }
    else {operand_1_temp = NULL;}

    char* operand_2_temp;
    if(i->operand_2 != NULL){
        operand_2_temp = (char*)malloc(strlen(i->operand_2) * sizeof(char));
        strcpy(operand_2_temp, i->operand_2);
    }
    else {operand_2_temp = NULL;}

    new_instruc.symbol = symbol_temp;
    new_instruc.opCode = opCode_temp;
    new_instruc.operand_1 = operand_1_temp;
    new_instruc.operand_2 = operand_2_temp;



    // create a Node and add new instruction
    struct Node new_Node = {0};
    new_Node.instruction = &new_instruc;
    new_Node.PC = pc;

    // add to linkedlist
    if(head.previous != NULL){
        new_Node.previous = head.previous;
        new_Node.next = &head;
        head.previous->next = &new_Node;
        head.previous = &new_Node;
        new_Node.PC = pc;

        return 0;

    }else if(head.instruction != NULL){
        head.previous = &new_Node;
        head.next = &new_Node;
        new_Node.next = &head;
        new_Node.previous = &head;
        new_Node.PC = pc;
        return 0;

    } else {
        head.instruction = &new_instruc;
        head.PC = pc;
        return 0;
    }

    free(new_instruc.symbol);
    free(new_instruc.opCode);
    free(new_instruc.operand_1);
    free(new_instruc.operand_2);
    return 1;
}

/**************************************************
**
** print the LinkedList of instructions
**
**************************************************/
void print_list(){

    struct Node* iterator = &head;
int i = 0;
    while(iterator->next != &head){
i++;
printf("--------------%d", i);
        //printf("%s\t%s\t%s\t%s\n",iterator->instruction->symbol,iterator->instruction->opCode,iterator->instruction->operand_1,iterator->instruction->operand_2);
        iterator = iterator->next;
    }

}















