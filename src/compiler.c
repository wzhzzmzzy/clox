#include <stdio.h>
#include <stdlib.h>
#include <string.h>
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

typedef void (*ParseFn)(bool canAssign);

/**
 * Token 对应的解析规则
 */
typedef struct {
  ParseFn prefix; // 作为一元运算符时的编译函数
  ParseFn infix; // 作为二元运算符时的编译函数
  Precedence precedence; // 优先级
} ParseRule;

typedef struct {
  Token name;
  int depth;
} Local;

/**
 * 编译器实例，其中缓存了编译器状态数据
 */
typedef struct {
  Local locals[UINT8_COUNT]; // 局部变量缓存
  int localCount; // 当前局部变量的数量，也就是当前变量的索引
  int scopeDepth; // 当前作用域深度
} Compiler;

Parser parser;
Compiler* current = NULL;
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

static bool check(TokenType type) {
  return parser.current.type == type;
}

static bool match(TokenType type) {
  if (!check(type)) return false;
  advance();
  return true;
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
 * @brief 输出一个 OP_LOOP 字节码，并记录当前位置和 loopStart 之间的偏移量
 * 
 * @param loopStart 循环起始点
 */
static void emitLoop(int loopStart) {
  emitByte(OP_LOOP);

  // +2 是算上了 OP_LOOP 后面操作数的长度 
  int offset = currentChunk()->count - loopStart + 2;
  if (offset > UINT16_MAX) error("Loop body too large.");

  emitByte((offset >> 8) & 0xff);
  emitByte(offset & 0xff);
}

/**
 * 输出一个 Jump 指令到 Chunk，
 * 后续的两个 0xff 字节是 Jump 指向的位置，
 * 因此最大的偏移量是 UINT16_MAX
 * 
 * @param instruction Jump 指令
 */
static int emitJump(uint8_t instruction) {
  emitByte(instruction);
  emitByte(0xff);
  emitByte(0xff);
  return currentChunk()->count - 2;
}

/**
 * @brief 添加一个 RETURN 字节码
 */
static void emitReturn() {
  emitByte(OP_RETURN);
}

/**
 * @brief 将一个值作为常量添加到常量列表，返回其索引
 * 
 * @param value 常量值
 * 
 * @return 常量列表索引
 */
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
 * 根据当前的字节码位置和 offset 计算需要跳转的距离，
 * 并补全 offset 位置 emitJump 的字节码
 * 
 * @param offset OP_JUMP_IF_FALSE 所在位置
 */
static void patchJump(int offset) {
  // -2 to adjust for the bytecode for the jump offset itself.
  int jump = currentChunk()->count - offset - 2;

  if (jump > UINT16_MAX) {
    error("Too much code to jump over.");
  }

  currentChunk()->code[offset] = (jump >> 8) & 0xff;
  currentChunk()->code[offset + 1] = jump & 0xff;
}

static void initCompiler(Compiler* compiler) {
  compiler->localCount = 0;
  compiler->scopeDepth = 0;
  current = compiler;
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

/**
 * 创建一个 block 的时候，作用域深度 ++
 */
static void beginScope() {
  current->scopeDepth++;
}

/**
 * 退出一个 block
 */
static void endScope() {
  // 作用域深度 --
  current->scopeDepth--;

  // 更新作用域深度后，POP 所有的局部变量
  while (
    current->localCount > 0 && 
    current->locals[current->localCount - 1].depth >
    current->scopeDepth
  ) {
    emitByte(OP_POP);
    current->localCount--;
  }
}


static void expression();
static void statement();
static void declaration();
static ParseRule* getRule(TokenType type);
static void parsePrecedence(Precedence precedence);

/**
 * 将变量名作为字符串存储为常量，返回索引
 * 
 * @param name 变量名 Token
 * 
 * @return 常量索引
 */
static uint8_t identifierConstant(Token* name) {
  return makeConstant(OBJ_VAL(copyString(name->start, name->length)));
}

/**
 * 对比变量名是否相同 
 */
static bool identifiersEqual(Token* a, Token* b) {
  if (a->length != b->length) return false;
  return memcmp(a->start, b->start, a->length) == 0;
}

/**
 * 遍历当前编译器的局部变量缓存，找到同名即可
 */
static int resolveLocal(Compiler* compiler, Token* name) {
  for (int i = compiler->localCount - 1; i >= 0; i--) {
    Local* local = &compiler->locals[i];
    if (identifiersEqual(name, &local->name)) {
      if (local->depth == -1) {
        error("Can't read local variable in its own initializer.");
      }
      return i;
    }
  }

  return -1;
}

/**
 * 添加一个局部变量到缓存当中
 */
static void addLocal(Token name) {
  if (current->localCount == UINT8_COUNT) {
    error("Too many local variables in function.");
    return;
  }

  Local* local = &current->locals[current->localCount++];
  local->name = name;
  local->depth = -1; // 初始化时，局部变量的深度为 -1
}

/**
 * 将局部变量名添加到编译器的缓存中
 */
static void declareVariable() {
  if (current->scopeDepth == 0) return;

  Token* name = &parser.previous;
  // 遍历所有局部变量，识别当前作用域内有没有同名变量
  for (int i = current->localCount - 1; i >= 0; i--) {
    Local* local = &current->locals[i];
    if (local->depth != -1 && local->depth < current->scopeDepth) {
      break; 
    }

    if (identifiersEqual(name, &local->name)) {
      error("Already a variable with this name in this scope.");
    }
  }

  addLocal(*name);
}

/**
 * @brief 解析变量名，如果是局部变量，则同时添加到编译器缓存内
 * 
 * @param errorMessage 没有解析到 TOKEN_IDENTIFIER 时输出的异常信息
 * @return 返回变量名的常量索引
 */
static uint8_t parseVariable(const char* errorMessage) {
  consume(TOKEN_IDENTIFIER, errorMessage);

  declareVariable();
  // 如果是局部变量，不需要返回变量名的常量索引，所以返回 0
  if (current->scopeDepth > 0) return 0;

  return identifierConstant(&parser.previous);
}

/**
 * 标记局部变量为已初始化：
 * 将当前局部变量的深度与当前作用域深度同步
 */
static void markInitialized() {
  current->locals[current->localCount - 1].depth = current->scopeDepth;
}

/**
 * @brief
 * 局部变量：标记为已初始化完成
 * 全局变量：添加一个 OP_DEFINE_GLOBAL global 到字节码中
 * 
 * @param global 全局变量名的常量索引
 */
static void defineVariable(uint8_t global) {
  if (current->scopeDepth > 0) {
    markInitialized();
    return;
  }

  emitBytes(OP_DEFINE_GLOBAL, global);
}

static void and_(bool canAssign) {
  int endJump = emitJump(OP_JUMP_IF_FALSE);

  emitByte(OP_POP);
  parsePrecedence(PREC_AND);

  patchJump(endJump);
}

/**
 * @brief
 * 编译二元运算符，输出字节码为 `(右侧表达式) (运算符)`，所以是递归执行
 */
static void binary(bool canAssign) {
  TokenType operatorType = parser.previous.type;
  ParseRule* rule = getRule(operatorType);
  // 先解析右侧表达式
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
static void literal(bool canAssign) {
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
static void grouping(bool canAssign) {
  expression();
  consume(TOKEN_RIGHT_PAREN, "Expect ')' after expression.");
}

static void number(bool canAssign) {
  double value = strtod(parser.previous.start, NULL);
  emitConstant(NUMBER_VAL(value));
}

/**
 * 或语句的中缀编译逻辑
 * if (left) return true;
 * else return right;
 * 所以 left == false 时使用 elseJump 跳过 true 的 endJump
 * 只有在 elseJump 时继续解析 right
 */
static void or_(bool canAssign) {
  int elseJump = emitJump(OP_JUMP_IF_FALSE);
  int endJump = emitJump(OP_JUMP);

  patchJump(elseJump);
  emitByte(OP_POP);

  parsePrecedence(PREC_OR);
  patchJump(endJump);
}

static void string(bool canAssign) {
  emitConstant(OBJ_VAL(copyString(
    // 去掉开头和结尾的 ""
    parser.previous.start + 1, 
    parser.previous.length - 2
  )));
}

static void namedVariable(Token name, bool canAssign) {
  uint8_t getOp, setOp;
  int arg = resolveLocal(current, &name);
  if (arg != -1) {
    getOp = OP_GET_LOCAL;
    setOp = OP_SET_LOCAL;
  } else {
    arg = identifierConstant(&name);
    getOp = OP_GET_GLOBAL;
    setOp = OP_SET_GLOBAL;
  }
  
  if (canAssign && match(TOKEN_EQUAL)) {
    expression();
    emitBytes(setOp, (uint8_t)arg);
  } else {
    emitBytes(getOp, (uint8_t)arg);
  }
}

static void variable(bool canAssign) {
  namedVariable(parser.previous, canAssign);
}

/**
 * @brief 一元操作符
 */
static void unary(bool canAssign) {
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
  [TOKEN_IDENTIFIER]    = {variable, NULL,   PREC_NONE},
  [TOKEN_STRING]        = {string,   NULL,   PREC_NONE},
  [TOKEN_NUMBER]        = {number,   NULL,   PREC_NONE},
  [TOKEN_AND]           = {NULL,     and_,   PREC_AND},
  [TOKEN_CLASS]         = {NULL,     NULL,   PREC_NONE},
  [TOKEN_ELSE]          = {NULL,     NULL,   PREC_NONE},
  [TOKEN_FALSE]         = {literal,  NULL,   PREC_NONE},
  [TOKEN_FOR]           = {NULL,     NULL,   PREC_NONE},
  [TOKEN_FUN]           = {NULL,     NULL,   PREC_NONE},
  [TOKEN_IF]            = {NULL,     NULL,   PREC_NONE},
  [TOKEN_NIL]           = {literal,  NULL,   PREC_NONE},
  [TOKEN_OR]            = {NULL,     or_,    PREC_OR},
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

  // 当前优先级低于赋值时，才检查是否可以赋值
  bool canAssign = precedence <= PREC_ASSIGNMENT;
  prefixRule(canAssign);

  // 在当前优先级范围内，递归执行中缀编译逻辑
  while (precedence <= getRule(parser.current.type)->precedence) {
    // 更新 Parser 状态到下一个 Token，用于中缀编译
    advance();
    // 读取中缀解析函数并执行
    ParseFn infixRule = getRule(parser.previous.type)->infix;
    infixRule(canAssign);
  }

  if (canAssign && match(TOKEN_EQUAL)) {
    error("Invalid assignment target.");
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

static void block() {
  while (!check(TOKEN_RIGHT_BRACE) && !check(TOKEN_EOF)) {
    declaration();
  }

  consume(TOKEN_RIGHT_BRACE, "Expect '}' after block.");
}

static void varDeclaration() {
  uint8_t global = parseVariable("Expect variable name.");

  if (match(TOKEN_EQUAL)) {
    expression();
  } else {
    emitByte(OP_NIL);
  }
  consume(TOKEN_SEMICOLON, "Expect ';' after variable declaration.");

  defineVariable(global);
}

static void expressionStatement() {
  expression();
  consume(TOKEN_SEMICOLON, "Expect ';' after expression.");
  emitByte(OP_POP);
}

/**
 * 解析 For 语句
 */
static void forStatement() {
  // 创建一个作用域
  beginScope();
  consume(TOKEN_LEFT_PAREN, "Expect '(' after 'for'.");

  // 编译初始化语句
  if (match(TOKEN_SEMICOLON)) {
    // No initializer.
  } else if (match(TOKEN_VAR)) {
    varDeclaration();
  } else {
    expressionStatement();
  }

  // 编译 Loop 条件部分，这里基本和 While 语句一致
  int loopStart = currentChunk()->count;
  // 如果有条件部分，则创建跳出循环的 OP_JUMP_IF_FALSE 指令
  int exitJump = -1;
  if (!match(TOKEN_SEMICOLON)) {
    expression();
    consume(TOKEN_SEMICOLON, "Expect ';' after loop condition.");

    // Jump out of the loop if the condition is false.
    exitJump = emitJump(OP_JUMP_IF_FALSE);
    emitByte(OP_POP); // Condition.
  }

  // 编译 For 语句的增量迭代部分，会在每一次迭代结束之后执行
  if (!match(TOKEN_RIGHT_PAREN)) {
    // 先执行迭代 Body 部分
    int bodyJump = emitJump(OP_JUMP);
    // 记录增量部分起始点，并编译增量部分的 expressionStatement
    int incrementStart = currentChunk()->count;
    expression();
    emitByte(OP_POP);
    consume(TOKEN_RIGHT_PAREN, "Expect ')' after for clauses.");
    // 在增量部分的最后，回到循环起点
    emitLoop(loopStart);
    // 将增量部分起始点设置为循环起点，保证每次进入循环先执行增量部分
    loopStart = incrementStart;
    patchJump(bodyJump);
  }

  statement();
  emitLoop(loopStart);

  // 补全退出点
  if (exitJump != -1) {
    patchJump(exitJump);
    emitByte(OP_POP); // Condition.
  }

  endScope();
}

static void ifStatement() {
  consume(TOKEN_LEFT_PAREN, "Expect '(' after 'if'.");
  expression();
  consume(TOKEN_RIGHT_PAREN, "Expect ')' after condition."); 

  int thenJump = emitJump(OP_JUMP_IF_FALSE);
  emitByte(OP_POP); // then 的情况下 pop
  statement();

  int elseJump = emitJump(OP_JUMP);

  patchJump(thenJump);
  emitByte(OP_POP); // else 的情况下 pop
  
  if (match(TOKEN_ELSE)) statement();
  patchJump(elseJump);
}

static void printStatement() {
  expression();
  consume(TOKEN_SEMICOLON, "Expect ';' after value.");
  emitByte(OP_PRINT);
}

static void whileStatement() {
  int loopStart = currentChunk()->count;
  consume(TOKEN_LEFT_PAREN, "Expect '(' after 'while'.");
  expression();
  consume(TOKEN_RIGHT_PAREN, "Expect ')' after condition.");

  int exitJump = emitJump(OP_JUMP_IF_FALSE);
  emitByte(OP_POP);
  statement();
  emitLoop(loopStart);

  patchJump(exitJump);
  emitByte(OP_POP);
}

static void synchronize() {
  parser.panicMode = false;

  while (parser.current.type != TOKEN_EOF) {
    if (parser.previous.type == TOKEN_SEMICOLON) return;
    switch (parser.current.type) {
      case TOKEN_CLASS:
      case TOKEN_FUN:
      case TOKEN_VAR:
      case TOKEN_FOR:
      case TOKEN_IF:
      case TOKEN_WHILE:
      case TOKEN_PRINT:
      case TOKEN_RETURN:
        return;

      default:
        ; // Do nothing.
    }

    advance();
  }
}

static void declaration() {
  if (match(TOKEN_VAR)) {
    varDeclaration();
  } else {
    statement();
  }

  if (parser.panicMode) synchronize();
}

static void statement() {
  if (match(TOKEN_PRINT)) {
    printStatement();
  } else if (match(TOKEN_FOR)) {
    forStatement();
  } else if (match(TOKEN_IF)) {
    ifStatement();
  } else if (match(TOKEN_WHILE)) {
    whileStatement();
  } else if (match(TOKEN_LEFT_BRACE)) {
    beginScope();
    block();
    endScope();
  } else {
    expressionStatement();
  }
}

bool compile(const char* source, Chunk* chunk) {
  // 初始化全局变量
  initScanner(source);
  Compiler compiler;
  initCompiler(&compiler);
  compilingChunk = chunk;

  parser.hadError = false;
  parser.panicMode = false;

  // 获取下一个 Token 并编译
  advance();

  while (!match(TOKEN_EOF)) {
    declaration();
  }

  endCompiler();
  return !parser.hadError;
}
