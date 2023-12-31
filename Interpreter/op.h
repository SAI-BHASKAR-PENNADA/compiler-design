// This file contains the classes which implement parse 
// trees of the calc operations. 
#ifndef OP_H
#define OP_H
#include <iostream>
#include <vector>
#include <map>
#include "lexer.h"


//////////////////////////////////////////
// Multi-Typed Result Returns
//////////////////////////////////////////
struct arrstruct {
    int size;
    void *ptr;
    bool isInt;
};

union ResultField
{
    int i;
    double r;
    struct arrstruct arr;
    void *ptr;
};


enum ResultType 
{
    VOID=0,
    INTEGER,
    REAL,
    ARRAY,
    CLASSDECLARATION,
    OBJECT
};


struct Result
{
    // the value and type of the result
    ResultField val;
    ResultType type;
};

// convert result types to strings
extern const char* RTSTR[];

// print result values
std::ostream& operator<<(std::ostream& os, const Result &result);

// A macro to extract the numeric result from Result
#define NUM_RESULT(res) ((res).type == INTEGER ? (res).val.i : (res).val.r)

// A macro to assign the correct numeric field
#define NUM_ASSIGN(res, n) ((res).type == INTEGER ? (res).val.i=(n) : (res).val.r=(n))


//////////////////////////////////////////
// Variable Storage
//////////////////////////////////////////
class RefEnv {
public:
    // constructor
    RefEnv();

    // declare a variable
    virtual void declare(const std::string &name, ResultType type);

    // check to see if a name exists in the environment
    virtual bool exists(const std::string &name);

    // retrieve a variable associative array style
    virtual Result& operator[](const std::string &name);

    // retrieve env of an object
    virtual RefEnv getEnv(const std::string &objName);

    // set new env for an object
    virtual void setEnv(const std::string &objName);

private:
    std::map<std::string, Result> _symtab;
    std::map<std::string, RefEnv> _objtab;
};


//////////////////////////////////////////
// Base Classes
//////////////////////////////////////////
class ParseTree
{
public:
    //constructor and destructor
    ParseTree(LexerToken &token);
    virtual ~ParseTree();

    // get the token of the parse tree
    virtual LexerToken token() const;

    // evaluate the parse tree
    virtual Result eval()=0;

    // print the tree (for debug purposes)
    virtual void print(int depth) const;

    // print the prefix for the tree
    virtual void print_prefix(int depth) const;
private:
    LexerToken _token;
};


// Base class for unary operations
class UnaryOp : public ParseTree
{
public:
    // constructor
    UnaryOp(LexerToken &_token);

    // destructor
    virtual ~UnaryOp();

    // give access to the child
    virtual ParseTree *child() const;
    virtual void child(ParseTree *_child);

    // print the tree with 1 child
    virtual void print(int depth) const;
protected:
    ParseTree *_child;    
};


// Base class for a binary operation
class BinaryOp : public ParseTree
{
public:
    //constructors
    BinaryOp(LexerToken &_token);

    //destructor
    virtual ~BinaryOp();

    // give access to the left child
    virtual ParseTree *left() const;
    virtual void left(ParseTree *child);

    // give access to the right child
    virtual ParseTree *right() const;
    virtual void right(ParseTree *child);

    // print the tree with 2 children
    virtual void print(int depth) const;

protected:
    ParseTree *_lchild;    
    ParseTree *_rchild;    
};


// Base class for n-arry operators (lists of children)
class NaryOp : public ParseTree
{
public:
    // constructor and destructor
    NaryOp(LexerToken _token);
    virtual ~NaryOp();

    // push a child onto the list
    virtual void push(ParseTree *child);

    // access iterators for the children
    virtual std::vector<ParseTree*>::const_iterator begin() const;
    virtual std::vector<ParseTree*>::const_iterator end() const;

    // print the tree
    virtual void print(int depth) const;
protected:
    std::vector<ParseTree*> _children;
};


//////////////////////////////////////////
// CalcOperations
//////////////////////////////////////////

// A calc program
class Program : public NaryOp
{
public:
    Program(LexerToken _token);
    virtual Result eval();
    virtual void print(int depth) const;
};


// An Add Operation
class Add : public BinaryOp
{
public:
    Add(LexerToken _token);
    virtual Result eval();
};


// A Subtract Operation
class Sub : public BinaryOp
{
public:
    Sub(LexerToken _token);
    virtual Result eval();
};


// A Multiply Operation
class Mul: public BinaryOp
{
public:
    Mul(LexerToken _token);
    virtual Result eval();
};


// A Divide Operation
class Div: public BinaryOp
{
public:
    Div(LexerToken _token);
    virtual Result eval();
};


// A Power Operation
class Pow: public BinaryOp
{
public:
    Pow(LexerToken _token);
    virtual Result eval();
};


// A Negate operations
class Neg: public UnaryOp 
{
public:
    Neg(LexerToken _token);
    virtual Result eval();
    virtual void print(int depth) const;
};


// A Literal Number
class Number: public ParseTree
{
public:
    Number(LexerToken _token);
    virtual Result eval();
protected:
    Result _val;
};


// A Variable Retrieval
class Var: public ParseTree
{
public:
    Var(LexerToken _token);
    virtual Result eval();
};


// A print operation
class Print: public UnaryOp 
{
public:
    Print(LexerToken _token);
    virtual Result eval();
};

class AlphaNumeric : public Print
{
public:
    AlphaNumeric(LexerToken _token);
    virtual Result eval();
};

class Println: public UnaryOp
{
public:
    Println(LexerToken _token);
    virtual Result eval();
};

// to declare an array
class ArrayInit: public NaryOp
{
public:
    ArrayInit(LexerToken _token);
    virtual Result eval();
};

// A SCANF operation
class ScanF : public ParseTree
{
public:
    ScanF(LexerToken _token);
    virtual Result eval();
};

// An IF statement
class IfStatement: public BinaryOp
{
public:
    IfStatement(LexerToken _token);
    virtual Result eval();
};

// A conditional op
class ConditionalOp: public BinaryOp
{
public:
    ConditionalOp(LexerToken _token);
    virtual Result eval();
};

// can have a bunch of statemetns - used for if/while blocks or for functiosn in future ?
class Statementblock: public NaryOp
{
public:
    Statementblock(LexerToken _token);
    virtual Result eval();
};

// A variable declaration operation
class VarDecl: public UnaryOp 
{
public:
    VarDecl(LexerToken _token);
    virtual Result eval();
};


// An Assignment Operation
class Assign : public BinaryOp
{
public:
    Assign(LexerToken _token);
    virtual Result eval();
};


// An array access operation
class ArrayDecl: public BinaryOp 
{
public:
    ArrayDecl(LexerToken _token);
    virtual Result eval();
};


// An array Access operation
class ArrayAccess: public BinaryOp 
{
public:
    ArrayAccess(LexerToken _token);
    virtual Result eval();
};

// An array assign operation
class ArrayAssign: public BinaryOp
{
public:
    ArrayAssign(LexerToken _token);
    virtual Result eval();
};

// An array index node
class ArrayIndex: public NaryOp
{
public:
    ArrayIndex(LexerToken _token);
    virtual Result eval();
};

// A class defintion operation
class ClassDefinition: public BinaryOp
{
public:
    ClassDefinition(LexerToken _token);
    virtual Result eval();
    bool isDerived;
    std::string parentName;
};

// variable declarations in a class
class VarDeclList: public NaryOp
{
public:
    VarDeclList(LexerToken _token);
    virtual Result eval();
};

class DefDeclList: public NaryOp
{
public:
    DefDeclList(LexerToken _token);
    virtual Result eval();
};

// An object creation operation
class ObjectCreation: public UnaryOp
{
public:
    ObjectCreation(LexerToken _token);
    virtual Result eval();
};

// An object access operation
class ObjectAccess: public NaryOp
{
public:
    ObjectAccess(LexerToken _token);
    virtual Result eval();
};

// A record definition operation
class RecordDef: public NaryOp 
{
public:
    RecordDef(LexerToken _token);
    virtual Result eval();
};


// A record access operation
class RecordAccess: public BinaryOp
{
public:
    RecordAccess(LexerToken _token);
    virtual Result eval();
};
#endif
