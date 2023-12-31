#include <iostream>
#include <cmath>
#include <stdexcept>
#include "lexer.h"
#include "op.h"

// global reference environment for variables
static RefEnv env;


//////////////////////////////////////////
// Helper Functions
//////////////////////////////////////////
static ResultType coerce(Result left, Result right) 
{
    // if the types match, there is no coercion
    if(left.type == right.type) return left.type;

    // if either left or right is void, so is the result
    if(left.type == VOID or right.type == VOID) return VOID;

    // perform type widening
    if((left.type == REAL and right.type == INTEGER) or 
       (left.type == INTEGER and right.type == REAL)) {
        return REAL;
    }

    //TODO: Technically, if we make it here we have an error. For now, we will
    //      just default to void. Eventually, we should report a type error.
    return VOID;
}


//////////////////////////////////////////
// Multi-Typed Result Returns
//////////////////////////////////////////

// handy string conversion for debugging
const char* RTSTR[] = { "VOID", "INTEGER", "REAL" };

// print result values
std::ostream& operator<<(std::ostream& os, const Result &result)
{
    // handle the numeric types
    if(result.type == INTEGER) return os << result.val.i;

    switch(result.type) {
        case VOID:
            break;
        case INTEGER:
            os << result.val.i;
            break;
        case REAL:
            os << result.val.r;
            break;
        default:
            break;
    }

    return os;
}


//////////////////////////////////////////
// Variable Storage
//////////////////////////////////////////

// constructor
RefEnv::RefEnv()
{
    // nothing to do
}


// declare a variable
void RefEnv::declare(const std::string &name, ResultType type)
{
    // names must be unique
    if(exists(name)) {
        throw std::runtime_error("Redeclaration of " + name);
    }

    // create the variable and add it to the table
    Result var;
    var.type = type;
    _symtab[name] = var;
}


// check to see if a name exists in the environment
bool RefEnv::exists(const std::string &name)
{
    return _symtab.find(name) != _symtab.end();
}


// retrieve a variable associative array style
Result& RefEnv::operator[](const std::string &name)
{
    // names must exist
    if(not exists(name)) {

        throw std::runtime_error(name + " not defined.");
    }
    return _symtab[name];
}

RefEnv RefEnv::getEnv(const std::string &objName) {
    if (_objtab.find(objName) != _objtab.end()) {
        return _objtab[objName];
    } else
        return *(new RefEnv());
}

void RefEnv::setEnv(const std::string &objName) {
    RefEnv *newenv = new RefEnv();
    env.declare(objName, OBJECT);
    _objtab[objName] = *newenv;
}


//////////////////////////////////////////
// UnaryOp Implementation
//////////////////////////////////////////

// constructor
UnaryOp::UnaryOp(LexerToken &_token) : ParseTree(_token)
{
    this->_child = nullptr;
}


// destructor
UnaryOp::~UnaryOp() 
{
    if(_child) {
        delete _child;  
    }
}


// give access to the child
ParseTree *UnaryOp::child() const
{
    return _child;
}


void UnaryOp::child(ParseTree *_child)
{
    this->_child = _child;
}


// print the tree with 1 child
void UnaryOp::print(int depth) const
{
    // print ourself
    ParseTree::print(depth);

    // print our child
    ParseTree *_child = child();
    if(_child) {
        _child->print(depth+1);
    }
}


//////////////////////////////////////////
// BinaryOp Implementation
//////////////////////////////////////////

//constructors
BinaryOp::BinaryOp(LexerToken &_token) : ParseTree(_token)
{
    _lchild = nullptr;
    _rchild = nullptr;
}


//destructor
BinaryOp::~BinaryOp()
{
    if(_lchild) {
        delete _lchild;
    }

    if(_rchild) {
        delete _rchild;
    }
}


// give access to the left child
ParseTree *BinaryOp::left() const
{
    return _lchild; 
}


void BinaryOp::left(ParseTree *child)
{
    this->_lchild = child; 
}


// give access to the right child
ParseTree *BinaryOp::right() const
{
    return _rchild;
}


void BinaryOp::right(ParseTree *child)
{
    this->_rchild = child;
}


// print the tree with 2 children
void BinaryOp::print(int depth) const
{
    ParseTree *l = left();
    ParseTree *r = right();

    // print the right child
    if(r) {
        r->print(depth+1);
    }

    // print ourself
    ParseTree::print(depth);

    // print the left child
    if(l) {
        l->print(depth+1);
    }
}


//////////////////////////////////////////
// NaryOp Implementation
//////////////////////////////////////////

// construuctor and destructor
NaryOp::NaryOp(LexerToken _token) : ParseTree(_token)
{
    // this space left intentionally blank
}


NaryOp::~NaryOp()
{
    // delete all the children
    for(auto itr = _children.begin(); itr != _children.end(); itr++) {
        delete *itr;
    }
}


// push a child onto the list
void NaryOp::push(ParseTree *child)
{
    _children.push_back(child);
}


// access iterators for the children
std::vector<ParseTree*>::const_iterator NaryOp::begin() const
{
    return _children.begin();
}


std::vector<ParseTree*>::const_iterator NaryOp::end() const
{
    return _children.end();
}


// print the tree
void NaryOp::print(int depth) const
{
    int n = _children.size();
    int m = n/2;

    // print the right hand side
    for(int i=n-1; i>=m; i--) {
        _children[i]->print(depth+1);
    }

    //print ourself
    ParseTree::print(depth);

    // print the left hand side
    for(int i=m-1; i>=0; i--) {
        _children[i]->print(depth+1);
    }
}


//////////////////////////////////////////
// Program implementation
//////////////////////////////////////////

Program::Program(LexerToken _token) : NaryOp(_token)
{
    // This space left intentionally blank
}

Result Program::eval()
{
    // evaluate each statement in the program
    for(auto itr = begin(); itr != end(); itr++) {
            (*itr)->eval();
    }

    // programs return void
    Result result;
    result.type = VOID;

    return result;
}


void Program::print(int depth) const
{
    int n = _children.size();
    int m = n/2;

    // print the right hand side
    for(int i=n-1; i>=m; i--) {
        _children[i]->print(depth+1);
    }

    //print ourself
    print_prefix(depth);
    std::cout << "PROGRAM" << std::endl;

    // print the left hand side
    for(int i=m-1; i>=0; i--) {
        _children[i]->print(depth+1);
    }
}


//////////////////////////////////////////
// Add implementation
//////////////////////////////////////////

Add::Add(LexerToken _token) : BinaryOp(_token)
{
    // This space left intentionally blank
}


Result Add::eval() 
{
    // evaluate the children
    Result l = left()->eval();
    Result r = right()->eval();

    // get the type of the result
    Result result;
    result.type = coerce(l, r);

    // perform the operation
    NUM_ASSIGN(result, NUM_RESULT(l) + NUM_RESULT(r));

    return result;
}


//////////////////////////////////////////
// Sub implementation
//////////////////////////////////////////

Sub::Sub(LexerToken _token) : BinaryOp(_token)
{
    // This space left intentionally blank
}


Result Sub::eval() 
{
    // evaluate the children
    Result l = left()->eval();
    Result r = right()->eval();

    // get the type of the result
    Result result;
    result.type = coerce(l, r);

    // perform the operation
    NUM_ASSIGN(result, NUM_RESULT(l) - NUM_RESULT(r));

    return result;
}


//////////////////////////////////////////
// Mul implementation
//////////////////////////////////////////

Mul::Mul(LexerToken _token) : BinaryOp(_token)
{
    // This space left intentionally blank
}


Result Mul::eval() 
{
    // evaluate the children
    Result l = left()->eval();
    Result r = right()->eval();

    // get the type of the result
    Result result;
    result.type = coerce(l, r);

    // perform the operation
    NUM_ASSIGN(result, NUM_RESULT(l) * NUM_RESULT(r));

    return result;
}


//////////////////////////////////////////
// Div implementation
//////////////////////////////////////////

Div::Div(LexerToken _token) : BinaryOp(_token)
{
    // This space left intentionally blank
}


Result Div::eval() 
{
    // evaluate the children
    Result l = left()->eval();
    Result r = right()->eval();

    // get the type of the result
    Result result;
    result.type = coerce(l, r);

    // perform the operation
    NUM_ASSIGN(result, NUM_RESULT(l) / NUM_RESULT(r));

    return result;
}


//////////////////////////////////////////
// Pow implementation
//////////////////////////////////////////

Pow::Pow(LexerToken _token) : BinaryOp(_token)
{
    // This space left intentionally blank
}


Result Pow::eval() 
{
    // evaluate the children
    Result l = left()->eval();
    Result r = right()->eval();

    // get the type of the result
    Result result;
    result.type = coerce(l, r);

    // perform the operation
    NUM_ASSIGN(result, pow(NUM_RESULT(l), NUM_RESULT(r)));

    return result;
}


//////////////////////////////////////////
// Neg implementation
//////////////////////////////////////////

Neg::Neg(LexerToken _token) : UnaryOp(_token)
{
    // This space left intentionally blank
}


Result Neg::eval()
{
    //eval the child and then negate it
    Result result = child()->eval();
    NUM_ASSIGN(result, -NUM_RESULT(result));

    return result;
}


void Neg::print(int depth) const
{
    print_prefix(depth);
    std::cout << "NEG: -" << std::endl;
    child()->print(depth+1);
}


//////////////////////////////////////////
// Number implementation
//////////////////////////////////////////

Number::Number(LexerToken _token) : ParseTree(_token)
{
    //get the number's value
    if(_token == INTLIT) {
        _val.type = INTEGER;
        _val.val.i = stoi(_token.lexeme);
    } else if(_token == REALLIT) {
        _val.type = REAL;
        _val.val.r = stod(_token.lexeme);
    }
}


Result Number::eval()
{
    return _val;
}


//////////////////////////////////////////
// ParseTree Implementation
//////////////////////////////////////////

ParseTree::ParseTree(LexerToken &token)
{
    this->_token = token;
}


ParseTree::~ParseTree()
{
    //nothing to do
}


// get the token of the parse tree
LexerToken ParseTree::token() const
{
    return _token;
}


// print the tree (for debug purposes)
void ParseTree::print(int depth) const
{
    print_prefix(depth);
    std::cout << TSTR[token().token] 
              << ": " << token().lexeme << std::endl;
}


// print the prefix for the tree
void ParseTree::print_prefix(int depth) const
{
    // no prefix for the root.
    if(depth == 0) return;

    for(int i=1; i<depth; i++) {
        std::cout << "  |";
    }

    if(depth > 1) {
        std::cout << "--+";
    } else {
        std::cout << "  +";
    }
}



//////////////////////////////////////////
// Var Implementation
//////////////////////////////////////////

Var::Var(LexerToken _token) : ParseTree(_token)
{
}


Result Var::eval()
{
    return env[token().lexeme];;
}


//////////////////////////////////////////
// Print Implementation
//////////////////////////////////////////
Print::Print(LexerToken _token) : UnaryOp(_token)
{
}


Result Print::eval()
{
    Result result;
    result.type = VOID;

    //print the result of the child
    std::cout << child()->eval() << std::endl;

    return result;
}

//////////////////////////////////////////
// ScanF Implementation
//////////////////////////////////////////
ScanF::ScanF(LexerToken _token) : ParseTree(_token) {}

Result ScanF::eval() {
    int userInput;
    double userIp;


    // check type of the variable
    Result var = env[token().lexeme];
    if (var.type == INTEGER) {
        std::cin >> userInput;
        var.val.i = userInput;
    } else if (var.type == REAL) {
        std::cin >> userIp;
        var.val.r = userIp;
    }
    env[token().lexeme] = var;
    Result res;
    return res;
}

//////////////////////////////////////////
// IF Implementation
//////////////////////////////////////////
IfStatement::IfStatement(LexerToken _token) : BinaryOp(_token){}

Result IfStatement::eval() {
    if (token() == IF) {
        // check if condition and then execute the if block
        if (left()->eval().val.i == 1 && left()->eval().type == VOID) {
            // evaluate the block
            right()->eval();
        }
    } else if(token() == WHILE) {
        while(left()->eval().val.i == 1 && left()->eval().type == VOID) {
            right()->eval();
        }
    }
    Result res;
    return res;
}

//////////////////////////////////////////
// ConditionalOp Implementation
//////////////////////////////////////////
ConditionalOp::ConditionalOp(LexerToken _token) : BinaryOp(_token){}

Result ConditionalOp::eval() {
    Result result;
    result.type = VOID;

    if (token().lexeme == "<") {
        result.val.i = left()->eval().val.i < right()->eval().val.i;
    } else if (token().lexeme == ">") {
        result.val.i = left()->eval().val.i > right()->eval().val.i;
    } else if (token().lexeme == "is") {
        result.val.i = left()->eval().val.i == right()->eval().val.i;
    } else if (token().lexeme == "is") {
        result.val.i = left()->eval().val.i != right()->eval().val.i;
    }
    return result;
}

//////////////////////////////////////////
// AlphaNumeric Implementation
//////////////////////////////////////////
AlphaNumeric::AlphaNumeric(LexerToken _token) : Print(_token) {}

Result AlphaNumeric::eval() {
    Result result;
    result.type = VOID;

    //print the alphanumberic string provided in the child
    std::cout << child()->token().lexeme;
    return result;
}


//////////////////////////////////////////
// Statementblock Implementation
//////////////////////////////////////////
Statementblock::Statementblock(LexerToken _token) : NaryOp(_token){}

Result Statementblock::eval() {
    for(auto itr = begin(); itr != end(); itr++) {
        (*itr)->eval();
    }
    Result res;
    res.type = VOID;
    return res;
}

//////////////////////////////////////////
// ArrayInit Implementation
//////////////////////////////////////////
ArrayInit::ArrayInit(LexerToken _token) : NaryOp(_token) {}

Result ArrayInit::eval() {
    //initialize an array in the env
    Result arr;
    int size = (*begin())->eval().val.i;

    if (token() == INTEGER_DECL) {
        arr.val.arr.isInt = true;         // set if it an int array or real array
        arr.val.arr.ptr = new int[size];  // create new array
    }
    else {
        arr.val.arr.isInt = false;
        arr.val.arr.ptr = new double[size];
    }
    arr.val.arr.size = size;

    // next add the Result to env
    std::string name = (*(begin() + 1))->token().lexeme;
    env.declare(name, ARRAY);
    env[name] = arr;
    env[name].type = ARRAY;
    Result res;
    return res;
}

//////////////////////////////////////////
// VarDecl Implementation
//////////////////////////////////////////
VarDecl::VarDecl(LexerToken _token) : UnaryOp(_token)
{
}

Result VarDecl::eval()
{
    ResultType var_type;
    Result result;
    result.type = VOID;

    //get the variable type
    switch(token().token)
    {
        case INTEGER_DECL:
            var_type = INTEGER;
            break;
        case REAL_DECL:
            var_type = REAL;
            break;
        default:
            break;
    }

    //perform the declaration
    env.declare(child()->token().lexeme, var_type);

    return result;
}


//////////////////////////////////////////
// Assign Impelementation
//////////////////////////////////////////
Assign::Assign(LexerToken _token) : BinaryOp(_token)
{
}


Result Assign::eval()
{
    // get the value and name to assign
    Result val = right()->eval();
    std::string name = left()->token().lexeme;

    //perform the assignment
    NUM_ASSIGN(env[name], NUM_RESULT(val));

    Result result;
    result.type = VOID;

    return result;
}


//////////////////////////////////////////
// ArrayDecl Impelementation
//////////////////////////////////////////
ArrayDecl::ArrayDecl(LexerToken _token) : BinaryOp(_token)
{
}


Result ArrayDecl::eval()
{

    //return void
    Result result;
    result.type = VOID;
    return result;
}


//////////////////////////////////////////
// ArrayAccess Implementation
//////////////////////////////////////////
ArrayAccess::ArrayAccess(LexerToken _token) : BinaryOp(_token) {}

Result ArrayAccess::eval()
{
    // left has the array name
    // right has the expression
    int index = right()->eval().val.i;
    std::string arrName = left()->token().lexeme;

    Result arr = env[arrName];
    int* arrayPtr = static_cast<int*>(arr.val.arr.ptr);
    Result res;
    res.type = arr.val.arr.isInt ? INTEGER : REAL;
    NUM_ASSIGN(res, arrayPtr[index]);

    return res;
}

//////////////////////////////////////////
// ArrayAssign Implementation
//////////////////////////////////////////
ArrayAssign::ArrayAssign(LexerToken _token) : BinaryOp( _token) {}

Result ArrayAssign::eval() {
    // token has var name
    // left has index expression
    // right has another expression
    Result rhs = right()->eval();
    Result index = left()->eval();
    int ind = index.val.i;
    std::string varName = token().lexeme;
    bool isint = env[varName].val.arr.isInt;
    if ((isint and rhs.type != INTEGER) or (not isint and rhs.type == INTEGER)) {
        std::cout<<"result type of expression does not match the array element type\n";
    } else {
        int *arrayPtr = static_cast<int*>(env[varName].val.arr.ptr);
        if (rhs.type == INTEGER)
            arrayPtr[ind] = rhs.val.i;
        else
            arrayPtr[ind] = rhs.val.r;
    }
    return rhs;
}


//////////////////////////////////////////
// ArrayIndex Implementation 
//////////////////////////////////////////
// An array index node
ArrayIndex::ArrayIndex(LexerToken _token) : NaryOp(_token)
{
}


Result ArrayIndex::eval()
{

    //return void
    Result result;
    result.type = VOID;
    return result;
}

//////////////////////////////////////////
// class definition Implementation
//////////////////////////////////////////
ClassDefinition::ClassDefinition(LexerToken _token) : BinaryOp(_token) {}
Result ClassDefinition::eval() {
    //left has variable declaration
    //right has function definitions
    Result classNode;
    classNode.val.ptr = this;
    env.declare(token().lexeme, CLASSDECLARATION);
    env[token().lexeme] = classNode;
    Result res;
    return res;
}

//////////////////////////////////////////
// object creation Implementation
//////////////////////////////////////////
ObjectCreation::ObjectCreation(LexerToken _token) : UnaryOp(_token) {}
Result ObjectCreation::eval() {

    // create reference env for the object
    std::string objectName = token().lexeme;
    env.setEnv(objectName);

    // create new entry for object in global env.. this holds the class name
    std::string className = child()->token().lexeme;
    char *ptr = new char[className.length() + 1];
    //std::strcpy(ptr, className.c_str());
    int i = 0;
    while (i < className.length() + 1) {
        ptr[i] = className[i];
        i++;
    }
    env[objectName].val.ptr = ptr;

    Result res;
    return res;
}

//////////////////////////////////////////
// object access Implementation
//////////////////////////////////////////
ObjectAccess::ObjectAccess(LexerToken _token) : NaryOp(_token) {}
Result ObjectAccess::eval() {
    // check if a function or variable
    if ((*(begin()+1)) == nullptr) {
        //it is a variable access
        std::string varName = (*begin())->token().lexeme;
        std::string objName = token().lexeme;
        // get class name from the env
        char *class_name = static_cast<char*>(env[objName].val.ptr);
        // now we have class name, look for the class node
        std::string className(class_name);
        ClassDefinition *def = (ClassDefinition*) env[className].val.ptr; // contains the class node
    } else if ((*(begin()+1))->token() == LPAREN) {
        //it is a function.. evaluate the function
        std::string objName = token().lexeme;
        std::string methodName = (*begin())->token().lexeme;

        // get class name from the env
        char *class_name = static_cast<char*>(env[objName].val.ptr);
        // now we have class name, look for the class node
        std::string className(class_name);
        ClassDefinition *def = (ClassDefinition*) env[className].val.ptr; // contains the class node

        // right child has the function list
        DefDeclList *deflist = (DefDeclList*) def->right();
        for (auto it = deflist->begin(); it != deflist->end(); it++) {
            if ((*it)->token().lexeme == methodName) {
                (*it)->eval();
                Result res;
                return res;
            }
        }

        //if function is not found look in the parent
        if (def->isDerived) {
            std::string parentClassName = def->parentName;
            def = (ClassDefinition*) env[parentClassName].val.ptr;
            deflist = (DefDeclList*) def->right();
            for (auto it = deflist->begin(); it != deflist->end(); it++) {
                if ((*it)->token().lexeme == methodName) {
                    (*it)->eval();
                    Result res;
                    return res;
                }
            }
        }

        throw std::runtime_error("Method: " + methodName + " not found in: " + objName);
    }
    Result res;
    return res;
}


//////////////////////////////////////////
// var declaration list Implementation
//////////////////////////////////////////
VarDeclList::VarDeclList(LexerToken _token) : NaryOp(_token) {}
Result VarDeclList::eval() {
    // evaluates all declarations and adds to the object's environment
    for (auto it = begin(); it != end(); it++) {
        (*it)->eval();
    }
    Result res;
    return res;
}

//////////////////////////////////////////
// function declaration list Implementation
//////////////////////////////////////////
DefDeclList::DefDeclList(LexerToken _token) : NaryOp(_token) {}
Result DefDeclList::eval() { Result res; return res;}

//////////////////////////////////////////
// RecordDef Implementation
//////////////////////////////////////////
RecordDef::RecordDef(LexerToken _token) : NaryOp(_token) 
{
}


Result RecordDef::eval()
{

    //return void
    Result result;
    result.type = VOID;
    return result;
}


//////////////////////////////////////////
// RecordAccess Implementation
//////////////////////////////////////////
RecordAccess::RecordAccess(LexerToken _token) : BinaryOp(_token)
{
}


Result RecordAccess::eval()
{

    //return void
    Result result;
    result.type = VOID;
    return result;
}
