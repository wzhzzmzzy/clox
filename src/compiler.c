#include <stdio.h>
#include <stdlib.h>
#include "common.h"
#include "compiler.h"
#include "scanner.h"

#ifdef DEBUG_PRINT_CODE
#include "debug.h"
#endif

/**
 * @brief 
 * 记录当前编译状态
 */
typedef struct {
  // 当前正在编译的 Token
  Token current;
  // 上一个编译完成的 Token
  Token previous;
  // 编译过程中是否发生了异常
  bool hadError;
  // Panic Mode 标志
  bool panicMode;
} Parser;

/**
 * @brief
 * Token 的优先级状态
 */
typedef enum {
  PREC_NONE,
  PREC_ASSIGNMENT,  // =
  PREC_OR,          // or
  PREC_AND,         // and
  PREC_EQUALITY,    // == !=
  PREC_COMPARISON,  // < > <= >=
  PREC_TERM,        // + -
  PREC_FACTOR,      // * /
  PREC_UNARY,       // ! -
  PREC_CALL,        // . ()
  PREC_PRIMARY
} Precedence;

typedef void (*ParseFn)();

/**
 * Token 对应的解析规则
 */
typedef struct {
  ParseFn prefix; // 作为一元运算符时的编译函数
  ParseFn infix; // 作为二元运算符时的编译函数
  Precedence precedence; // 优先级
} ParseRule;

Parser parser;
Chunk* compilingChunk;

static Chunk* currentChunk() {
  return compilingChunk;
}

/**
 * @brief 进入 Panic Mode，同时输出一些异常信息
 * 
 * @param token Token 内容
 * @param message 报错信息
 */
static void errorAt(Token* token, const char* message) {
  if (parser.panicMode) return;
  parser.panicMode = true;
  fprintf(stderr, "[line %d] Error", token->line);

  if (token->type == TOKEN_EOF) {
    fprintf(stderr, " at end");
  } else if (token->type == TOKEN_ERROR) {
    // Nothing.
  } else {
    fprintf(stderr, " at '%.*s'", token->length, token->start);
  }

  fprintf(stderr, ": %s\n", message);
  parser.hadError = true;
}

/**
 * @brief 解析 Token 完成后，转字节码时遇到异常
 * 
 * @param message 报错信息
 */
static void error(const char* message) {
  errorAt(&parser.previous, message);
}

/**
 * @brief 解析当前 Token 时遇到异常
 * 
 * @param message 报错信息
 */
static void errorAtCurrent(const char* message) {
  errorAt(&parser.current, message);
}

/**
 * @brief 解析下一个 Token，更新 Parser 状态
 */
static void advance() {
  parser.previous = parser.current;

  for (;;) {
    parser.current = scanToken();
    if (parser.current.type != TOKEN_ERROR) break;
    
    // 处理 TOKEN_ERROR
    errorAtCurrent(parser.current.start);
  }
}

/**
 * @brief 断言检查当前 Token 类型
 * 
 * @param type 
 * @param message 异常信息
 */
static void consume(TokenType type, const char* message) {
  if (parser.current.type == type) {
    advance();
    return;
  }

  errorAtCurrent(message);
}

/**
 * @brief 输出一个字节码到 Chunk
 * @param byte 
 */
static void emitByte(uint8_t byte) {
  writeChunk(currentChunk(), byte, parser.previous.line);
}

/**
 * @brief 输出两个连续的字节码到 Chunk
 * @param byte1 
 * @param byte2 
 */
static void emitBytes(uint8_t byte1, uint8_t byte2) {
  emitByte(byte1);
  emitByte(byte2);
}

/**
 * @brief 添加一个 RETURN 字节码
 */
static void emitReturn() {
  emitByte(OP_RETURN);
}

static uint8_t makeConstant(Value value) {
  int constant = addConstant(currentChunk(), value);
  if (constant > UINT8_MAX) {
    error("Too many constants in one chunk.");
    return 0;
  }

  return (uint8_t)constant;
}

/**
 * @brief 添加 CONSTANT 字节码
 */
static void emitConstant(Value value) {
  emitBytes(OP_CONSTANT, makeConstant(value));
}

/**
 * @brief 结束编译流程
 */
static void endCompiler() {
  emitReturn();
#ifdef DEBUG_PRINT_CODE
  if (!parser.hadError) {
    disassembleChunk(currentChunk(), "code");
  }
#endif
}

static void expression();
static ParseRule* getRule(TokenType type);
static void parsePrecedence(Precedence precedence);

/**
 * @brief
 * 编译二元运算符，输出字节码为 `(右侧表达式) (运算符)`，所以是递归执行
 */
static void binary() {
  TokenType operatorType = parser.previous.type;
  ParseRule* rule = getRule(operatorType);
  parsePrecedence((Precedence)(rule->precedence + 1));

  switch (operatorType) {
    // != 实现为 !(a==b) 的语法糖
    case TOKEN_BANG_EQUAL:    emitBytes(OP_EQUAL, OP_NOT); break;
    case TOKEN_EQUAL_EQUAL:   emitByte(OP_EQUAL); break;
    case TOKEN_GREATER:       emitByte(OP_GREATER); break;
    // >= 实现为 !(a<b) 的语法糖
    case TOKEN_GREATER_EQUAL: emitBytes(OP_LESS, OP_NOT); break;
    case TOKEN_LESS:          emitByte(OP_LESS); break;
    // <= 实现为 !(a>b) 的语法糖
    case TOKEN_LESS_EQUAL:    emitBytes(OP_GREATER, OP_NOT); break;
    case TOKEN_PLUS:          emitByte(OP_ADD); break;
    case TOKEN_MINUS:         emitByte(OP_SUBTRACT); break;
    case TOKEN_STAR:          emitByte(OP_MULTIPLY); break;
    case TOKEN_SLASH:         emitByte(OP_DIVIDE); break;
    default: return; // Unreachable.
  }
}

/**
 * @brief 处理语言内置的字面量，目前只支持 True False Nil
 */
static void literal() {
  switch (parser.previous.type) {
    case TOKEN_FALSE: emitByte(OP_FALSE); break;
    case TOKEN_NIL: emitByte(OP_NIL); break;
    case TOKEN_TRUE: emitByte(OP_TRUE); break;
    default: return; // Unreachable.
  }
}

/**
 * @brief 处理括号内的表达式
 */
static void grouping() {
  expression();
  consume(TOKEN_RIGHT_PAREN, "Expect ')' after expression.");
}

static void number() {
  double value = strtod(parser.previous.start, NULL);
  emitConstant(NUMBER_VAL(value));
}

static void string() {
  emitConstant(OBJ_VAL(copyString(
    // 去掉开头和结尾的 ""
    parser.previous.start + 1, 
    parser.previous.length - 2
  )));
}

/**
 * @brief 一元操作符
 */
static void unary() {
  TokenType operatorType = parser.previous.type;

  // 先解析后面的操作数
  parsePrecedence(PREC_UNARY);

  // 解析完成后添加操作符
  switch (operatorType) {
    case TOKEN_BANG: emitByte(OP_NOT); break;
    case TOKEN_MINUS: emitByte(OP_NEGATE); break;
    default: return; // Unreachable.
  }
}

ParseRule rules[] = {
  [TOKEN_LEFT_PAREN]    = {grouping, NULL,   PREC_NONE},
  [TOKEN_RIGHT_PAREN]   = {NULL,     NULL,   PREC_NONE},
  [TOKEN_LEFT_BRACE]    = {NULL,     NULL,   PREC_NONE}, 
  [TOKEN_RIGHT_BRACE]   = {NULL,     NULL,   PREC_NONE},
  [TOKEN_COMMA]         = {NULL,     NULL,   PREC_NONE},
  [TOKEN_DOT]           = {NULL,     NULL,   PREC_NONE},
  [TOKEN_MINUS]         = {unary,    binary, PREC_TERM},
  [TOKEN_PLUS]          = {NULL,     binary, PREC_TERM},
  [TOKEN_SEMICOLON]     = {NULL,     NULL,   PREC_NONE},
  [TOKEN_SLASH]         = {NULL,     binary, PREC_FACTOR},
  [TOKEN_STAR]          = {NULL,     binary, PREC_FACTOR},
  [TOKEN_BANG]          = {unary,    NULL,   PREC_NONE},
  [TOKEN_BANG_EQUAL]    = {NULL,     binary, PREC_EQUALITY},
  [TOKEN_EQUAL]         = {NULL,     NULL,   PREC_NONE},
  [TOKEN_EQUAL_EQUAL]   = {NULL,     binary, PREC_EQUALITY},
  [TOKEN_GREATER]       = {NULL,     binary, PREC_COMPARISON},
  [TOKEN_GREATER_EQUAL] = {NULL,     binary, PREC_COMPARISON},
  [TOKEN_LESS]          = {NULL,     binary, PREC_COMPARISON},
  [TOKEN_LESS_EQUAL]    = {NULL,     binary, PREC_COMPARISON},
  [TOKEN_IDENTIFIER]    = {NULL,     NULL,   PREC_NONE},
  [TOKEN_STRING]        = {string,   NULL,   PREC_NONE},
  [TOKEN_NUMBER]        = {number,   NULL,   PREC_NONE},
  [TOKEN_AND]           = {NULL,     NULL,   PREC_NONE},
  [TOKEN_CLASS]         = {NULL,     NULL,   PREC_NONE},
  [TOKEN_ELSE]          = {NULL,     NULL,   PREC_NONE},
  [TOKEN_FALSE]         = {literal,  NULL,   PREC_NONE},
  [TOKEN_FOR]           = {NULL,     NULL,   PREC_NONE},
  [TOKEN_FUN]           = {NULL,     NULL,   PREC_NONE},
  [TOKEN_IF]            = {NULL,     NULL,   PREC_NONE},
  [TOKEN_NIL]           = {literal,  NULL,   PREC_NONE},
  [TOKEN_OR]            = {NULL,     NULL,   PREC_NONE},
  [TOKEN_PRINT]         = {NULL,     NULL,   PREC_NONE},
  [TOKEN_RETURN]        = {NULL,     NULL,   PREC_NONE},
  [TOKEN_SUPER]         = {NULL,     NULL,   PREC_NONE},
  [TOKEN_THIS]          = {NULL,     NULL,   PREC_NONE},
  [TOKEN_TRUE]          = {literal,  NULL,   PREC_NONE},
  [TOKEN_VAR]           = {NULL,     NULL,   PREC_NONE},
  [TOKEN_WHILE]         = {NULL,     NULL,   PREC_NONE},
  [TOKEN_ERROR]         = {NULL,     NULL,   PREC_NONE},
  [TOKEN_EOF]           = {NULL,     NULL,   PREC_NONE},
};

/**
 * @brief 按照给定的优先级，从ParseRule表格中获取解析函数，从而编译后续的表达式
 * 
 * @param precedence Toekn优先级
 */
static void parsePrecedence(Precedence precedence) {
  // 更新 Parser 状态到下一个 Token，用于前缀编译
  advance();
  // 获取前缀编译函数，如果没有前缀编译函数会抛出异常
  ParseFn prefixRule = getRule(parser.previous.type)->prefix;
  if (prefixRule == NULL) {
    error("Expect expression.");
    return;
  }

  // 执行前缀编译
  prefixRule();

  // 在当前优先级范围内，递归执行中缀编译逻辑
  while (precedence <= getRule(parser.current.type)->precedence) {
    // 更新 Parser 状态到下一个 Token，用于中缀编译
    advance();
    // 读取中缀解析函数并执行
    ParseFn infixRule = getRule(parser.previous.type)->infix;
    infixRule();
  }
}

/**
 * @brief
 * 按照指定的 Token 获取解析规则
 */
static ParseRule* getRule(TokenType type) {
  return &rules[type];
}

/**
 * @brief 编译当前 Parser 记录的 Token，可以是任意表达式
 */
static void expression() {
  parsePrecedence(PREC_ASSIGNMENT);
}

bool compile(const char* source, Chunk* chunk) {
  // 初始化全局变量
  initScanner(source);
  compilingChunk = chunk;

  parser.hadError = false;
  parser.panicMode = false;

  // 获取下一个 Token 并编译
  advance();
  expression();
  // 结束编译
  consume(TOKEN_EOF, "Expect end of expression.");
  endCompiler();
  return !parser.hadError;
}
