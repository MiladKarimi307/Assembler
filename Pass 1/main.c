#define _CRT_SECURE_NO_WARNINGS
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>

#define SIZE 30

struct node {
    char* name;
    int address;
    struct node* next;
};

struct instruction_line {
    char* symbol;
    char* opCode;
    char* operand_1;
    char* operand_2;
}instruct;

const int cap = 32768;                // memory capacity
struct node table[SIZE];              // hash table
int program_counter[2] = {0x0,0x0};   // Program counter
int line = 0;                         // line number

void Tokenize( char* s, struct instruction_line* instruct);
int is_directive( char* s );
int hash( char* name );
int add_symbol(char* symbol, int ma);
void print_table();
int calc_address(const struct instruction_line *i);
struct node* find_symbol(char* sym);

int main( int argc, char* argv[] )
{
    //initialize hash table
    for( int i = 0; i < SIZE; i++ ) {
        table[i].name = NULL;
        table[i].address = 0;
        table[i].next = NULL;
    }

    // check correct # of args
    if(argc != 2){
        printf("ERROR: Invalid use of argument\n");
        return 1;
    }

    FILE* inputFile;
    inputFile = fopen( argv[1], "r" );

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
                if(add_symbol(instruct.symbol, program_counter[1])){
                    printf( "ERROR: (line %d) duplicate symbol name \"%s\"\n",line, instruct.symbol );
                    exit(1);
                }
            } else program_counter[0] += 3;
            program_counter[1] = program_counter[0];
            if(program_counter[1] > cap){
                    printf("ERROR: (line %d) the memory limit exceeded\n", line);
                    exit(1);
            }
            if(!strcmp(instruct.opCode, "END")) flag++;
        }
    }
    if(flag != 2) printf("ERROR: \"END\" directive is missing form the source file\n");

    print_table();

    fclose( inputFile );
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
    printf("%s\t%X\n", temp->name, temp->address);

    if (table[x].name != NULL) {
        struct node* head = &table[x];
        // look for duplicate in linked list
        while (1) {

            // return 1 if duplicate found
            if (strcmp(head->name, temp->name) == 0) {
                free(temp->name);
                free(temp);
                return 1;
            }
            // move head forward
            if (head->next) {
                head = head->next;
                continue;
            } else {
            // add temp as new head and insert into table
                head->next = temp;
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
            struct node* head = &table[index];
            printf(" (%s, %X) -> ", head->name, head->address);
            while (head->next != NULL) {
               head = head->next;
               printf(" (%s, %X) ->", head->name, head->address);
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
        struct node* head = &table[i];
        if(!strcmp(sym, head->name)) return head;
        while(head->next != NULL){
            head = head->next;
            if(!strcmp(sym, head->name)) return head;
        }
    }
    return NULL;
}
