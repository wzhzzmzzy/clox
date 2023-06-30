#include <stdio.h>
#include <string.h>

#include "common.h"
#include "scanner.h"

/**
 * @brief 双指针扫描器，实时缓存当前扫描状态，用于生成 Token
 */
typedef struct {
  // 当前 Token 的起始位置
  const char* start;
  // 当前扫描到的位置，扫描完成后会是 Token 的结束位置 +1
  const char* current;
  // 当前 Token 的行号
  int line;
} Scanner;

Scanner scanner;

void initScanner(const char* source) {
  scanner.start = source;
  scanner.current = source;
  scanner.line = 1;
}

static bool isAlpha(char c) {
  return (c >= 'a' && c <= 'z') ||
         (c >= 'A' && c <= 'Z') ||
          c == '_';
}

static bool isDigit(char c) {
  return c >= '0' && c <= '9';
}

/**
 * @brief 是否已经扫描到了代码结束为止
 */
static bool isAtEnd() {
  return *scanner.current == '\0';
}

/**
 * @brief 返回当前字符，并递增 Scanner 的偏移到下一个位置
 * @return char Scanner 中当前位置的代码
 */
static char advance() {
  scanner.current++;
  return scanner.current[-1];
}

/**
 * @brief 返回当前字符
 * @return char Scanner 中当前位置的代码
 */
static char peek() {
  return *scanner.current;
}

/**
 * @brief 返回下一个字符
 * @return char Scanner 中下一个位置的代码
 */
static char peekNext() {
  if (isAtEnd()) return '\0';
  return scanner.current[1];
}

/**
 * @brief 与 Scanner 中当前位置的字符做比较，返回是否一致
 * @param expected 比较当前位置的字符
 * @return true 一致
 * @return false 不一致
 */
static bool match(char expected) {
  if (isAtEnd()) return false;
  if (*scanner.current != expected) return false;
  scanner.current++;
  return true;
}

/**
 * @brief 在当前位置，创建并返回一个指定类型的 Token
 * @param type 指定 TokenType
 * @return Token 
 */
static Token makeToken(TokenType type) {
  Token token;
  token.type = type;
  token.start = scanner.start;
  token.length = (int)(scanner.current - scanner.start);
  token.line = scanner.line;
  return token;
}

/**
 * @brief 发送 Token 消息
 * @param message 消息
 * @return Token 
 */
static Token errorToken(const char* message) {
  Token token;
  token.type = TOKEN_ERROR;
  token.start = message;
  token.length = (int)strlen(message);
  token.line = scanner.line;
  return token;
}

/**
 * @brief 
 * 跳过后续的空格字符和注释，遇到换行符递增行号
 */
static void skipWhitespace() {
  for (;;) {
    char c = peek();
    switch (c) {
      case ' ':
      case '\r':
      case '\t':
        advance();
        break;
      case '\n':
        scanner.line++;
        advance();
        break;
      case '/':
        if (peekNext() == '/') {
          // A comment goes until the end of the line.
          while (peek() != '\n' && !isAtEnd()) advance();
        } else {
          return;
        }
        break;
      default:
        return;
    }
  }
}

/**
 * @brief 
 * 比对当前 Token 的后缀和指定关键字是否一致
 * @param start 后缀所在位置的偏移量，例如 and 的 nd 偏移量是 1
 * @param length 后缀长度
 * @param rest 后缀内容
 * @param type 如果后缀一致，返回 type，否则返回 IDENTIFIER
 * @return TokenType 
 */
static TokenType checkKeyword(int start, int length, const char* rest, TokenType type) {
  if (scanner.current - scanner.start == start + length &&
      memcmp(scanner.start + start, rest, length) == 0) {
    return type;
  }
  
  return TOKEN_IDENTIFIER;
}

static TokenType identifierType() {
  switch (scanner.start[0]) {
    case 'a': return checkKeyword(1, 2, "nd", TOKEN_AND);
    case 'c': return checkKeyword(1, 4, "lass", TOKEN_CLASS);
    case 'e': return checkKeyword(1, 3, "lse", TOKEN_ELSE);
    case 'f':
      if (scanner.current - scanner.start > 1) {
        switch (scanner.start[1]) {
          case 'a': return checkKeyword(2, 3, "lse", TOKEN_FALSE);
          case 'o': return checkKeyword(2, 1, "r", TOKEN_FOR);
          case 'u': return checkKeyword(2, 1, "n", TOKEN_FUN);
        }
      }
      break;
    case 'i': return checkKeyword(1, 1, "f", TOKEN_IF);
    case 'n': return checkKeyword(1, 2, "il", TOKEN_NIL);
    case 'o': return checkKeyword(1, 1, "r", TOKEN_OR);
    case 'p': return checkKeyword(1, 4, "rint", TOKEN_PRINT);
    case 'r': return checkKeyword(1, 5, "eturn", TOKEN_RETURN);
    case 's': return checkKeyword(1, 4, "uper", TOKEN_SUPER);
    case 't':
      if (scanner.current - scanner.start > 1) {
        switch (scanner.start[1]) {
          case 'h': return checkKeyword(2, 2, "is", TOKEN_THIS);
          case 'r': return checkKeyword(2, 2, "ue", TOKEN_TRUE);
        }
      }
      break;
    case 'v': return checkKeyword(1, 2, "ar", TOKEN_VAR);
    case 'w': return checkKeyword(1, 4, "hile", TOKEN_WHILE);
  }

  return TOKEN_IDENTIFIER;
}

/**
 * @brief 生成标识符 Token，包括变量名、函数名等
 * @return Token 
 */
static Token identifier() {
  while (isAlpha(peek()) || isDigit(peek())) advance();
  return makeToken(identifierType());
}

/**
 * @brief 读取后续的所有数字
 * @return Token 数字表达式的 Token
 */
static Token number() {
  while (isDigit(peek())) advance();

  // Look for a fractional part.
  if (peek() == '.' && isDigit(peekNext())) {
    // Consume the ".".
    advance();

    while (isDigit(peek())) advance();
  }

  return makeToken(TOKEN_NUMBER);
}

/**
 * @brief 读取后续的所有字符作为字符串，直到下一个「"」字符
 * @return Token 字符串 Token
 */
static Token string() {
  while (peek() != '"' && !isAtEnd()) {
    if (peek() == '\n') scanner.line++;
    advance();
  }

  if (isAtEnd()) return errorToken("Unterminated string.");

  // The closing quote.
  advance();
  return makeToken(TOKEN_STRING);
}

/**
 * @brief 扫描下一个 Token 并返回
 * @return Token 
 */
Token scanToken() {
  skipWhitespace();
  scanner.start = scanner.current;

  // 到头了，EOF
  if (isAtEnd()) return makeToken(TOKEN_EOF);

  char c = advance();
  // 字母开头就是 identifier
  if (isAlpha(c)) return identifier();
  // 数字开头就是 number
  if (isDigit(c)) return number();

  // 特殊符号情况，按照枚举生成 Token
  switch (c) {
    case '(': return makeToken(TOKEN_LEFT_PAREN);
    case ')': return makeToken(TOKEN_RIGHT_PAREN);
    case '{': return makeToken(TOKEN_LEFT_BRACE);
    case '}': return makeToken(TOKEN_RIGHT_BRACE);
    case ';': return makeToken(TOKEN_SEMICOLON);
    case ',': return makeToken(TOKEN_COMMA);
    case '.': return makeToken(TOKEN_DOT);
    case '-': return makeToken(TOKEN_MINUS);
    case '+': return makeToken(TOKEN_PLUS);
    case '/': return makeToken(TOKEN_SLASH);
    case '*': return makeToken(TOKEN_STAR);
    case '!':
      return makeToken(match('=') ? TOKEN_BANG_EQUAL : TOKEN_BANG);
    case '=':
      return makeToken(match('=') ? TOKEN_EQUAL_EQUAL : TOKEN_EQUAL);
    case '<':
      return makeToken(match('=') ? TOKEN_LESS_EQUAL : TOKEN_LESS);
    case '>':
      return makeToken(match('=') ? TOKEN_GREATER_EQUAL : TOKEN_GREATER);
    case '"': return string();
  }

  return errorToken("Unexpected character.");
}
