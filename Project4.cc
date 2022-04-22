#include <iostream>
#include <cstdio>
#include <cstdlib>
#include <map>
#include <algorithm>
#include <vector>
#include <queue>
#include "lexer.h"
#include "inputbuf.h"
#include "execute.h"

// Author: Haili Liang

using namespace std;

/*
****************************************************
 GLOBAL VARIABLES
****************************************************
*/
LexicalAnalyzer lexer;
vector<InstructionNode*> instructions;
map<string, int> variable_locations;
map<int, int> const_locations;


/*
****************************************************
 PROTOTYPES
****************************************************
 */
// Parse
InstructionNode* parse_program();
void parse_var_section();
void parse_id_list();
InstructionNode* parse_body();
InstructionNode* parse_stmt_list();
InstructionNode* parse_stmt();
InstructionNode* parse_assign_stmt();
InstructionNode* parse_expr();
int parse_primary();
ArithmeticOperatorType parse_op();
InstructionNode* parse_output_stmt();
InstructionNode* parse_input_stmt();
InstructionNode* parse_while_stmt();
InstructionNode* parse_if_stmt();
InstructionNode* parse_condition();
ConditionalOperatorType parse_relop();
InstructionNode* parse_switch_stmt();
InstructionNode* parse_for_stmt();
InstructionNode* parse_case_list(int);
InstructionNode* parse_case(int);
InstructionNode* parse_default_case();
void parse_inputs();
void parse_num_list();

//Helper
Token expect(TokenType);
void syntax_error();
int check_id(Token);
int check_const(Token);

/*
****************************************************
FUNCTIONS
****************************************************
*/

struct InstructionNode * parse_generate_intermediate_representation(){
    return parse_program();
}
InstructionNode* parse_program(){
    parse_var_section();
    InstructionNode* inst =  parse_body();
    parse_inputs();
    expect(END_OF_FILE);
    return inst;
}
void parse_var_section(){
    parse_id_list();
    expect(SEMICOLON);
}
void parse_id_list(){
    auto t1 = lexer.peek(1);
    auto t2 = lexer.peek(2);
    auto id = expect(ID);
    check_id(id);

    if(t1.token_type == ID && t2.token_type == COMMA){
        expect(COMMA);  
        parse_id_list();
    }else if(t1.token_type == ID && t2.token_type == SEMICOLON){ 
        return;
    }else{
        syntax_error();
    }
}
InstructionNode* parse_body(){
    InstructionNode * curr = new InstructionNode;
    expect(LBRACE);
    curr = parse_stmt_list();
    expect(RBRACE);
    return curr;
}
InstructionNode* parse_stmt_list(){
    auto t11 = lexer.peek(1);
    InstructionNode* curr = parse_stmt();
    InstructionNode* end  = curr;
    if((t11.token_type == WHILE)
        || (t11.token_type == IF)
        || (t11.token_type == SWITCH)
        || (t11.token_type == FOR)
        ){
        while(end->next != nullptr){
            end = end->next;
        }
    }
    auto t21 = lexer.peek(1);
    auto t22 = lexer.peek(2);
    if(
        (t21.token_type == ID && t22.token_type == EQUAL)
        || (t21.token_type == WHILE)
        || (t21.token_type == IF)
        || (t21.token_type == SWITCH)
        || (t21.token_type == FOR)
        || (t21.token_type == OUTPUT)
        || (t21.token_type == INPUT)
        ){
        InstructionNode* inst_list= parse_stmt_list();
        end->next = inst_list;
        return curr;
    }else if(t21.token_type == RBRACE){
        end->next = nullptr;
        return curr;
    }else{
        syntax_error();
    }
}
InstructionNode* parse_stmt(){
    InstructionNode * curr = new InstructionNode;
    auto t1 = lexer.peek(1);
    auto t2 = lexer.peek(2);
    auto t3 = lexer.peek(3);

    if(t1.token_type== ID && t2.token_type == EQUAL){
        curr = parse_assign_stmt();
    }else if(t1.token_type == WHILE){
        curr = parse_while_stmt();
    }else if(t1.token_type == IF){
        curr = parse_if_stmt();
    }else if(t1.token_type == SWITCH){
        curr = parse_switch_stmt();
    }else if(t1.token_type == FOR){
        curr = parse_for_stmt();
    }else if(t1.token_type == OUTPUT){
        curr = parse_output_stmt();
    }else if(t1.token_type == INPUT){
        curr = parse_input_stmt();
    }else{
        syntax_error();
    }
    return curr;
}
InstructionNode* parse_assign_stmt(){
    InstructionNode* res = new InstructionNode;
    
    auto t = expect(ID);
    expect(EQUAL);
    
    auto t1 = lexer.peek(1);
    auto t2 = lexer.peek(2);

    if(
    (t1.token_type == ID || t1.token_type == NUM)
    &&( t2.token_type == PLUS 
        ||t2.token_type == MINUS 
        ||t2.token_type == MULT
        ||t2.token_type == DIV)
    ){
        res = parse_expr();
    }else if(
    (t1.token_type == ID || t1.token_type == NUM)
    && t2.token_type == SEMICOLON
    ){
        res->assign_inst.operand1_index =  parse_primary();
        res->assign_inst.op = OPERATOR_NONE;
    }else{
        syntax_error();
    }
    res->assign_inst.left_hand_side_index = check_id(t);
    expect(SEMICOLON);
    res->type = ASSIGN;
    return res;
}
InstructionNode* parse_expr(){
    InstructionNode* res = new InstructionNode;
    res->assign_inst.operand1_index =  parse_primary();
    res->assign_inst.op =  parse_op();
    res->assign_inst.operand2_index = parse_primary();
    return res;
}
int parse_primary(){
    auto t1 = lexer.peek(1);
    int loc = -1;

    if(t1.token_type == ID){
        auto t = expect(ID);
        loc = check_id(t);
    }else if (t1.token_type == NUM){
        auto t = expect(NUM);
        loc = check_const(t);
    }else{
        syntax_error();
    }
    return loc;
}
ArithmeticOperatorType parse_op(){
    auto t1 = lexer.peek(1);
    if(t1.token_type == PLUS){
        expect(PLUS);
        return OPERATOR_PLUS;
    }else if(t1.token_type == MINUS){
        expect(MINUS);
        return OPERATOR_MINUS;
    }else if (t1.token_type == MULT){
        expect(MULT);
        return OPERATOR_MULT;
    }else if(t1.token_type == DIV){
        expect(DIV);
        return OPERATOR_DIV;
    }else{
        syntax_error();
    }
}
InstructionNode* parse_output_stmt(){
    InstructionNode* res = new InstructionNode;
    res->type = OUT;
    expect(OUTPUT);
    auto t = expect(ID);
    res->output_inst.var_index = check_id(t);
    expect(SEMICOLON);
    return res;
}
InstructionNode* parse_input_stmt(){
    InstructionNode* res = new InstructionNode;
    res->type = IN;
    expect(INPUT);
    auto t = expect(ID);
    res->input_inst.var_index = check_id(t);
    expect(SEMICOLON);
    return res;
}
InstructionNode* parse_while_stmt(){
    InstructionNode* cjp = new InstructionNode;
    InstructionNode* jp = new InstructionNode;
    InstructionNode* end = new InstructionNode;
    InstructionNode* tmp = new InstructionNode;

    jp->type = JMP;
    jp->next = end;
    jp->jmp_inst.target = cjp;

    cjp->type = CJMP;
    cjp->cjmp_inst.target = end;

    end->type = NOOP;
    end->next = nullptr;
    
    expect(WHILE);
    tmp = parse_condition();
    cjp->cjmp_inst.condition_op = tmp ->cjmp_inst.condition_op;
    cjp->cjmp_inst.operand1_index = tmp->cjmp_inst.operand1_index;
    cjp->cjmp_inst.operand2_index =  tmp->cjmp_inst.operand2_index;
    cjp->next =  parse_body();

    tmp = cjp;
    while(tmp->next != nullptr){
        tmp = tmp->next;
    }
    tmp->next = jp;

    return cjp;
}
InstructionNode* parse_if_stmt(){
    InstructionNode* cjp = new InstructionNode;
    InstructionNode* end = new InstructionNode;
    InstructionNode* stmtList_end = new InstructionNode;
    end->type = NOOP;
    end->next = nullptr;
    expect(IF);
    cjp = parse_condition();
    cjp->type = CJMP;
    cjp->next = parse_body();
    stmtList_end = cjp;
    while(stmtList_end->next != nullptr){
        stmtList_end = stmtList_end->next;
    }
    stmtList_end->next = end;
    cjp->cjmp_inst.target = end;

    return cjp;
}
InstructionNode* parse_condition(){
    InstructionNode* curr = new InstructionNode;
    curr->type = CJMP;
    curr->cjmp_inst.operand1_index = parse_primary();
    curr->cjmp_inst.condition_op = parse_relop();
    curr->cjmp_inst.operand2_index = parse_primary();
    return curr;

}
ConditionalOperatorType parse_relop(){
    auto t1 = lexer.peek(1);
    if(t1.token_type == GREATER){
        expect(GREATER);
        return CONDITION_GREATER;
    }else if(t1.token_type == LESS){
        expect(LESS);
        return CONDITION_LESS;
    }else if (t1.token_type == NOTEQUAL){
        expect(NOTEQUAL);
        return CONDITION_NOTEQUAL;
    }
}
InstructionNode* parse_switch_stmt(){
    InstructionNode* switch_end = new InstructionNode;
    switch_end->type = NOOP;
    switch_end->next = nullptr;
    expect(SWITCH);
    auto t = expect(ID);
    int var_loc = check_id(t);
    expect(LBRACE);
    InstructionNode* csLst_head =  parse_case_list(var_loc);
    InstructionNode* cslst_end = csLst_head;
    while(cslst_end->next!= nullptr){
        InstructionNode* stmtLst_end = cslst_end->cjmp_inst.target;
        while(stmtLst_end->next != nullptr){
            stmtLst_end = stmtLst_end->next;
        }
        stmtLst_end->next = switch_end;
        cslst_end = cslst_end->next;
    }
    InstructionNode* stmtLst_end = cslst_end->cjmp_inst.target;
    while(stmtLst_end->next != nullptr){
            stmtLst_end = stmtLst_end->next;
    }
    stmtLst_end->next = switch_end;
    
    auto t1 = lexer.peek(1);
    if(t1.token_type == DEFAULT){
        InstructionNode* def = parse_default_case();
        cslst_end->next = def;
        InstructionNode* default_end = def->jmp_inst.target;
        while(default_end->next != nullptr){
            default_end = default_end->next;
        }
        default_end->next = switch_end;
        def->next = switch_end;
    }else if(t1.token_type == RBRACE){
        cslst_end->next = switch_end;  
    }else{
        syntax_error();
    }
    expect(RBRACE);
    return csLst_head;
}
InstructionNode* parse_for_stmt(){
    expect(FOR);
    expect(LPAREN);
    InstructionNode* assign1 = parse_assign_stmt();
    InstructionNode* cjp =  parse_condition();
    InstructionNode* jp = new InstructionNode;
    InstructionNode* for_end = new InstructionNode;
    for_end->type = NOOP;
    for_end->next = nullptr;
    expect(SEMICOLON);
    InstructionNode* assign2 =  parse_assign_stmt();
    expect(RPAREN);
    InstructionNode* stmtLst_head = parse_body();
    InstructionNode* stmtLst_end = stmtLst_head;
    while (stmtLst_end->next != nullptr){
        stmtLst_end = stmtLst_end->next;
    }
    cjp->next = stmtLst_head;
    cjp->cjmp_inst.target = for_end;
    jp->type = JMP;
    jp->next = for_end;
    jp->jmp_inst.target = cjp;
    assign1->next = cjp;
    stmtLst_end->next = assign2;
    assign2->next = jp;
    return assign1;
}
InstructionNode* parse_case_list(int var_loc){
    InstructionNode* cjp = parse_case(var_loc);

    auto t1 = lexer.peek(1);
    if(t1.token_type == CASE){

        cjp->next =  parse_case_list(var_loc);
        return cjp;
    }else if(t1.token_type == RBRACE || t1.token_type == DEFAULT){
        cjp->next = nullptr;
        return cjp;
    }
    else syntax_error();
}
InstructionNode* parse_case(int var_loc){
    InstructionNode* curr = new InstructionNode;
    curr->type = CJMP;
    curr->cjmp_inst.condition_op = CONDITION_NOTEQUAL;
    curr->cjmp_inst.operand1_index = var_loc;
    expect(CASE);
    curr->cjmp_inst.operand2_index = check_const(expect(NUM));
    expect(COLON);
    curr->cjmp_inst.target = parse_body();
    return curr;
}
InstructionNode* parse_default_case(){

    expect(DEFAULT);
    expect(COLON);
    InstructionNode* jp = new InstructionNode;
    jp->type = JMP;
    jp->jmp_inst.target = parse_body();
    jp->next = nullptr;
    return jp;
}
void parse_inputs(){
    parse_num_list();
}
void parse_num_list(){
    auto num_t = expect(NUM);
    inputs.push_back(stoi(num_t.lexeme));
    auto t1 = lexer.peek(1);
    if(t1.token_type == NUM){
        parse_num_list();
    }else if(t1.token_type == END_OF_FILE){
        return;
    }else syntax_error();
}

// Helper Functions
void syntax_error(){
    cout << "SYNTAX ERROR";
    exit(1);
}
Token expect(TokenType exp){
    auto t = lexer.GetToken();
    if(t.token_type != exp){
        syntax_error();
    }
    return t;
}
int check_id(Token t){
    string a  = t.lexeme;
    auto search = variable_locations.find(a);
    if(search == variable_locations.end()){
        int loc = next_available;
        next_available++;
        variable_locations[a] = loc;
        mem[loc] = 0;
        return loc;
    }else{
        return variable_locations[a];
    }   
}
int check_const(Token t){
    int num = stoi(t.lexeme);
    auto search = const_locations.find(num);
    if(search == const_locations.end()){
        int loc = next_available;
        next_available++;
        const_locations[num] = loc;
        mem[loc] = num;
        return loc;
    }else{
        return const_locations[num];
    }
}
