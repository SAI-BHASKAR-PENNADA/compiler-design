#include <iostream>
#include <iomanip>
#include <sstream>
#include "parser.h"
#include "op.h"

//////////////////////////////////////////
// Parser Implementation
//////////////////////////////////////////

// initalize the lexer and get the first token
Parser::Parser(Lexer &_lexer) : _lexer(_lexer)
{
    // Load up the lexer's token buffer.
    next();
}


// parse the program
ParseTree *Parser::parse()
{
    return parse_program();
}


//token matches
bool Parser::has(Token tok)
{
    return _curtok == tok;
}


void Parser::must_be(Token tok)
{
    // Throw an exception if we don't match.
    if(not has(tok)) {
        // throw a parse error
        throw ParseError{_curtok};
    }
}


//advance the lexer
void Parser::next()
{
    _curtok = _lexer.next();
}


// get the current token
LexerToken Parser::curtok() const
{
    return _curtok;
}


// non-terminal parse functions

/*
 * < Program >     ::= < Program > < Statement > 
 *                      | < Statement >
 */
ParseTree *Parser::parse_program()
{
    Program *result = new Program(curtok());

    // Technically, this is not LL(1), but it is easy enough to handle 
    // this with a while loop
    while(not has(TEOF)) {
        result->push(parse_statement());
    }
    return result;
}


/*
 * < Statement >   ::= < Identifier > < Statement' > NEWLINE
 *                     | < Var-Decl > NEWLINE
 *                     | < Print > NEWLINE
 *                     | < Expression > NEWLINE
 */
ParseTree *Parser::parse_statement()
{
    ParseTree *result;
    if(has(IDENTIFIER)) {
        // statement which begin with an identifier
        LexerToken variableName = curtok();
        next();

        if (has(ISA)) {
            result = parse_obj_decl(variableName);
        } else if (has(DOT)) {
            result = parse_obj_access(variableName);
        } else if (has(LBRACKET)) {
            return parse_array_assign(variableName);
        } else {
            result = parse_statement_prime(new Var(variableName));
        }
    } else if(has(INTEGER_DECL) or has(REAL_DECL)) {
        result = parse_var_decl();
    } else if(has(IF) || has(WHILE)) {
        result = parse_if();
    } else if(has(PRINT)) {
        result = parse_print();
    } else if(has(SCANF)) {
        result = parse_scanf();
    } else if (has(CLASS)) {
        result = parse_class();
    } else {
        result = parse_expression();
    }

    // handle the newline at the end of the statement
    must_be(NEWLINE);
    next();
    return result;
}

ParseTree *Parser::parse_obj_access(LexerToken _token) {
    next();
    must_be(IDENTIFIER);
    ObjectAccess *objectAccess = new ObjectAccess(_token);
    objectAccess->push(new Var(curtok()));
    next();

    // can be a variable or a function
    if (has(LPAREN)) {
        // push this - to know if accessed element is variable or a function
        objectAccess->push(new Var(curtok()));
        next();

        // add all arguments now
        while (not has(RPAREN)) {
            objectAccess->push(new Var(curtok()));
            next();
            if (has(COMMA))
                next();
            else
                must_be(RPAREN);
        }

        must_be(RPAREN);
        next();
    }
    return objectAccess;
}

ParseTree *Parser::parse_class() {
    next();
    must_be(IDENTIFIER);
    ClassDefinition *def = new ClassDefinition(curtok());
    next();

    // for derived classes
    if (has(DERIVED)) {
        def->isDerived = true;
        next();
        def->parentName = curtok().lexeme;
        next();
    } else {
        def->isDerived = false;
    }

    must_be(ISTO);
    next();

    must_be(NEWLINE);
    next();

    // store all variable declarations in left child
    def->left(parse_var_decl_list());

    // store all def declarations in right child
    def->right(parse_def_decl_list());

    must_be(CLASSEND);
    next();
    return def;
}

ParseTree *Parser::parse_var_decl_list() {
    VarDeclList *decList = new VarDeclList(curtok());

    // check for access modifier
    while (has(PUBLIC) or has(PRIVATE)) {
        decList->push(new Var(curtok()));
        next();
        decList->push(parse_var_decl());
        must_be(NEWLINE);
        next();
    }
    return decList;
}

ParseTree *Parser::parse_def_decl_list() {
    DefDeclList *declList = new DefDeclList(curtok());

    while (has(DEF)) {
        declList->push(parse_def());
        must_be(NEWLINE);
        next();
    }
    return declList;
}

ParseTree *Parser::parse_def() {
    next();

    //can re-use program node to store all statements in the function
    Program *def = new Program(curtok());
    next();

    must_be(LPAREN);
    next();
    while (not has(RPAREN))
        next();
    next();

    must_be(ISTO);
    next();

    must_be(NEWLINE);
    next();

    while (not has(ENDDEF)) {
        def->push(parse_statement());
    }
    next();
    return def;
}

ParseTree *Parser::parse_obj_decl(LexerToken _token) {
    next();
    ObjectCreation *obj = new ObjectCreation(_token);
    must_be(IDENTIFIER);
    obj->child(new Var(curtok()));
    next();
    return obj;
}

/*
 * < Statement' >  ::= EQUAL < Expression > 
 *                     | < Expression' >
 */
ParseTree *Parser::parse_statement_prime(ParseTree *left)
{
    if(has(EQUAL)) {
        Assign *result;
        result = new Assign(curtok());
        next();
        result->left(left);
        result->right(parse_expression());
        return result;
    } else {
        return parse_expression_prime(left);
    }
}


/*
 * < Var-Decl >    ::= < Type > < Identifier >
 */
ParseTree *Parser::parse_var_decl()
{
    LexerToken integerOrReal = curtok();
    next();

    if (has(LBRACKET)) {
        next();
        return parse_array_init(integerOrReal);
    }
    VarDecl *result = new VarDecl(integerOrReal);
    must_be(IDENTIFIER);
    result->child(new Var(curtok()));
    next();

    return result;
}


ParseTree *Parser::parse_array_init(LexerToken _token) {
    ArrayInit *arrinit = new ArrayInit(_token);
    arrinit->push(parse_number());
    must_be(RBRACKET);
    next();
    must_be(IDENTIFIER);
    arrinit->push(new Var(curtok()));
    next();
    return arrinit;
}

ParseTree *Parser::parse_array_assign(LexerToken varname) {
    next();
    ArrayAssign *arrasgn = new ArrayAssign(varname);
    arrasgn->left(parse_expression());
    must_be(RBRACKET);
    next();
    must_be(EQUAL);
    next();
    arrasgn->right(parse_expression());
    must_be(NEWLINE);
    next();
    return arrasgn;
}

/*
 * < Print >       ::= PRINT < Expression >
 */
ParseTree *Parser::parse_print()
{
    Print *result = new Print(curtok());
    next();
    if (has(DOUBLE_QUOTES)) {
           next();
           result->child(parse_alpha_numeric());
    } else {
        result->child(parse_expression());
    }
    return result;
}

ParseTree *Parser::parse_alpha_numeric() {
    std::string printable = "";
    while (not has(DOUBLE_QUOTES)) {
        printable = printable + curtok().lexeme + " ";
        next();
    }
    next();
    LexerToken tok;
    tok.lexeme = printable;
    AlphaNumeric *alpha = new AlphaNumeric(tok);
    alpha->child(new Var(tok));
    return alpha;
}

ParseTree *Parser::parse_scanf() {
    next();
    must_be(LPAREN);
    next();
    must_be(IDENTIFIER);
    ScanF *scanner =  new ScanF(curtok());
    next();
    must_be(RPAREN);
    next();
    return scanner;
}

ParseTree *Parser::parse_if() {
    IfStatement *ifs = new IfStatement(curtok());
    next();
    // add the condition to the left child and the statemnt block to the right child
    ifs->left(parse_condition_expression());
    Statementblock *ifblock = new Statementblock(curtok());
    if (ifs->token().token == IF) {
        while (not has(ENDIF)) {
            // add all the statement to right child of the if node
            ifblock->push(parse_statement());
        }
        must_be(ENDIF);
    } else {
        while (not has(ENDWHILE)) {
            // add all the statement to right child of the if node
            ifblock->push(parse_statement());
        }
        must_be(ENDWHILE);
    }
    next();
    ifs->right(ifblock);
    return ifs;
}

ParseTree *Parser::parse_condition_expression() {
    ParseTree *result;
    must_be(LPAREN);
    next();
    // need to write logic for collecting <expression> operator <expression>
    auto it = parse_expression();
    ConditionalOp *op = new ConditionalOp(curtok());
    next();
    op->left(it);
    op->right(parse_expression());
    must_be(RPAREN);
    next();
    must_be(ISTO);
    next();
    must_be(NEWLINE);
    next();
    return op;
}

/*
 * < Expression >  ::= < Term > < Expression' >
 */
ParseTree *Parser::parse_expression()
{
    ParseTree *left = parse_term();
    return parse_expression_prime(left);
}


/*
 * < Expression' > ::= PLUS < Term  > < Expression' >
 *                     | MINUS < Term > < Expression' >
 *                     | ""
 */
ParseTree *Parser::parse_expression_prime(ParseTree *left)
{
    if(has(PLUS)) {
        // start the parse tree
        Add *result = new Add(curtok());
        next();

        //get the children
        result->left(left);
        result->right(parse_term());
        return parse_expression_prime(result);
    } else if(has(MINUS)) {
        // start the parse tree
        Sub *result = new Sub(curtok());
        next();

        // get the children
        result->left(left);
        result->right(parse_term());
        return parse_expression_prime(result);
    }

    // nothing to do for the empty case
    return left;
}


/*
 * < Term >        ::= < Factor > < Term' >
 */
ParseTree *Parser::parse_term()
{
    ParseTree *left = parse_factor();
    return parse_term_prime(left);
}


/*
 * < Term' >       ::= TIMES  < Factor > < Term' >
 *                     | DIVIDE < Factor > < Term' >
 *                     | ""
 */
ParseTree *Parser::parse_term_prime(ParseTree *left)
{
    if(has(TIMES)) {
        // start the parse tree
        Mul *result = new Mul(curtok());
        next();

        // get the children
        result->left(left);
        result->right(parse_factor());
        return parse_term_prime(result);
    } else if(has(DIVIDE)) {
        // start the parse tree
        Div *result = new Div(curtok());
        next();

        result->left(left);
        result->right(parse_factor());
        return parse_term_prime(result);
    }

    //empty string, nothing to do
    return left;
}


/*
 * < Factor >      ::= < Base > POW < Factor >
 *                     | < Base >
 */
ParseTree *Parser::parse_factor()
{
    ParseTree *left = parse_base();
    if(has(POW)) {
        //create the parse tree
        Pow *result = new Pow(curtok());
        next();

        // get the children
        result->left(left);
        result->right(parse_factor());
        return result;
    }
    return left;
}


/*
 * < Base >        ::= LPAREN < Expression > RPAREN
 *                     | MINUS < Expression > 
 *                     | < Number >
 */
ParseTree *Parser::parse_base()
{
    if(has(LPAREN)) {
        next();
        ParseTree *result = parse_expression();
        must_be(RPAREN);
        next();
        return result;
    } else if(has(MINUS)) {
        Neg *result = new Neg(curtok());
        next();
        result->child(parse_expression());
        return result;
    } else {
        return parse_number();
    }
}


/*
 * < Number >      ::= INTLIT
 *                     | REALLIT
 *                     | IDENTIFIER
 */
ParseTree *Parser::parse_number()
{
    ParseTree *result;

    if(has(IDENTIFIER)) {
        LexerToken variableName = curtok();
        next();
        if (not has(LBRACKET)) {
            result = new Var(variableName);
        } else {

            result = new Var(variableName);
            next();
            ArrayAccess *res = new ArrayAccess(variableName);
            res->left(result);
            res->right(parse_expression());
            must_be(RBRACKET);
            next();
            return res;
        }
    } else if(has(INTLIT)) {
        result = new Number(curtok());
        next();
    } else {
        must_be(REALLIT);
        result = new Number(curtok());
        next();
    }

    return result;
}


//////////////////////////////////////////
// ParseError Implementation
//////////////////////////////////////////

ParseError::ParseError(LexerToken &_tok)
{
    // capture the token
    this->_tok = _tok;

    // generate the message
    std::ostringstream os;
    os << "Unexpected Token " << _tok;

    _msg = os.str();
}


const char* ParseError::what() const noexcept
{

    return _msg.c_str();
}


LexerToken ParseError::token() const
{
    return _tok;
}
