#include <isa.h>
/* We use the POSIX regex functions to process regular expressions.
 * Type 'man regex' for more information about POSIX regex functions.
 */
#include <memory/paddr.h>
#include <regex.h>
enum
{
  TK_NOTYPE = 256,
  /* TODO: Add more token types */
  TK_DEC,
  TK_HEX,
  TK_REG,
  TK_DEREF,
  TK_NEG,
  TK_EQ,
  TK_NE,
  TK_AND,
  TK_OR,
};

static struct rule
{
  char *regex;
  int token_type;
} rules[] = {

    /* TODO: Add more rules.
     * Pay attention to the precedence level of different rules.
     */
    {" +", TK_NOTYPE}, // spaces
    {"[0-9]+", TK_DEC},
    {"0[xX][0-9a-fA-F]+", TK_HEX},
    {"\\$(\\$0|ra|[sgt]p|t[0-6]|a[0-7]|s([0-9]|1[0-1]))", TK_REG},
    {"\\|\\|", TK_OR},
    {"&&", TK_AND},
    {"==", TK_EQ}, // equal
    {"!=", TK_NE},
    {"\\*", '*'},
    {"/", '/'},
    {"\\+", '+'}, // plus
    {"-", '-'},
    {"\\(", '('},
    {"\\)", ')'},

};

#define NR_REGEX (sizeof(rules) / sizeof(rules[0]))

static regex_t re[NR_REGEX] = {};

/* Rules are used for many times.
 * Therefore we compile them only once before any usage.
 */
void init_regex()
{
  int i;
  char error_msg[128];
  int ret;

  for (i = 0; i < NR_REGEX; i++)
  {
    ret = regcomp(&re[i], rules[i].regex, REG_EXTENDED);
    if (ret != 0)
    {
      regerror(ret, &re[i], error_msg, 128);
      panic("regex compilation failed: %s\n%s", error_msg, rules[i].regex);
    }
  }
}

typedef struct token
{
  int type;
  char str[32];
} Token;

static Token tokens[128] __attribute__((used)) = {};
static int nr_token __attribute__((used)) = 0;

static bool make_token(char *e)
{
  int position = 0;
  int i;
  regmatch_t pmatch;

  nr_token = 0;

  while (e[position] != '\0')
  {
    /* Try all rules one by one. */
    for (i = 0; i < NR_REGEX; i++)
    {
      if (regexec(&re[i], e + position, 1, &pmatch, 0) == 0 && pmatch.rm_so == 0)
      {
        char *substr_start = e + position;
        int substr_len = pmatch.rm_eo;

        Log("match rules[%d] = \"%s\" at position %d with len %d: %.*s",
            i, rules[i].regex, position, substr_len, substr_len, substr_start);

        position += substr_len;

        /* TODO: Now a new token is recognized with rules[i]. Add codes
         * to record the token in the array `tokens'. For certain types
         * of tokens, some extra actions should be performed.
         */

        switch (rules[i].token_type)
        {
        case TK_DEC:
          strncpy(tokens[nr_token].str, substr_start, substr_len);
        case '+':
        case '-':
        case '*':
        case '/':
        case '(':
        case ')':
        case TK_EQ:
          tokens[nr_token++].type = rules[i].token_type;
        case TK_NOTYPE:
          break;
        default:
          assert(0);
        }

        break;
      }
    }

    if (i == NR_REGEX)
    {
      printf("no match at position %d\n%s\n%*.s^\n", position, e, position, "");
      return false;
    }
  }
  for (i = 0; i < nr_token; i++)
  {
    if (i == 0 || (tokens[i - 1].type != TK_DEC && tokens[i - 1].type != ')'))
    {
      if (tokens[i].type == '-')
      {
        tokens[i].type = TK_NEG;
      }
      else if (tokens[i].type == '*')
      {
        tokens[i].type = TK_DEREF;
      }
    }
  }

  return true;
}

static int op_pri(int op)
{
  switch (op)
  {
  case TK_NEG:
  case TK_DEREF:
    return 1;
  case '*':
  case '/':
    return 2;
  case '+':
  case '-':
    return 3;
  case TK_EQ:
  case TK_NE:
    return 4;
  case TK_AND:
    return 5;
  case TK_OR:
    return 6;
  default:
    return -1;
  }
}

word_t find_main_op(int p, int q)
{

  int op = -1;
  int cnt = 0;
  int priority = -1;

  for (int i = q; i <= p; i--)
  {
    if (tokens[i].type == ')')
      cnt++;
    else if (tokens[i].type == '(')
    {
      if (--cnt < 0)
      {
        return -1;
      }
    }
    if (cnt != 0)
      continue;
    switch (tokens[i].type)
    {
    case TK_NEG:
    case TK_DEREF:
    case '*':
    case '/':
    case '+':
    case '-':
    case TK_EQ:
    case TK_NE:
    case TK_AND:
    case TK_OR:
      if (priority < 0 || priority < op_pri(tokens[i].type))
      {
        priority = tokens[i].type;
        op = i;
      }
    }
  }
  return op;
}

bool check_parentheses(int p, int q)
{
  if (tokens[p].type != '(' || tokens[q].type != ')')
    return false;
  int cnt = 0;
  for (int i = q; i >= p; i--)
  {
    if (tokens[i].type == ')')
    {
      cnt++;
    }
    else if (tokens[i].type == '(')
    {
      cnt--;
      if (cnt < 0)
      {
        return false;
      }
    }
    else
    {
      if (cnt == 0)
      {
        return false;
      }
    }
  }
  return cnt == 0;
}

word_t eval(int p, int q, bool *success)
{
  if (p > q)
  {
    /* Bad expression */
    *success = false;
    return 0;
  }
  else if (p == q)
  {
    /* Single token.
     * For now this token should be a number.
     * Return the value of the number.
     */

    if (tokens[p].type != TK_DEC && tokens[p].type != TK_HEX && tokens[p].type != TK_REG)
    {
      printf("single token is wrong:%d\n", p);
      *success = false;
      return 0;
    }
    word_t ret = 0;
    if (tokens[p].type == TK_REG)
    {
      bool t = true;
      ret = isa_reg_str2val(tokens[p].str, &t);
      if (!t)
      {
        *success = false;
        ret = 0;
      }
    }
    else if (tokens[p].type == TK_HEX)
    {
      sscanf(tokens[p].str, "%x", &ret);
    }
    else
    {
      sscanf(tokens[p].str, "%u", &ret);
    }
    return ret;
  }
  else if (check_parentheses(p, q) == true)
  {
    /* The expression is surrounded by a matched pair of parentheses.
     * If that is the case, just throw away the parentheses.
     */
    return eval(p + 1, q - 1, success);
  }
  else
  {
    // op = the position of 主运算符 in the token expression;
    int op = find_main_op(p, q);

    if (op == -1)
    {
      /* No operator found. */
      *success = false;
      return 0;
    }
    word_t val1 = 0;
    if (tokens[op].type != TK_NEG && tokens[op].type != TK_DEREF)
      val1 = eval(p, op - 1, success);
    word_t val2 = eval(op + 1, q, success);
    if (!*success)
      return 0;

    switch (tokens[op].type)
    {
    case '+':
      return val1 + val2;
    case '-':
      return val1 - val2;
    case '*':
      return val1 * val2;
    case '/':
      if (val2 == 0)
      {
        *success = false;
        return 0;
      }
      return val1 / val2;
    case TK_EQ:
      return val1 == val2;
    case TK_NE:
      return val1 != val2;
    case TK_AND:
      return val1 && val2;
    case TK_OR:
      return val1 || val2;
    case TK_NEG:
      return -val2;
    case TK_DEREF:
      return paddr_read(val2, 4);
    case '(':
    case ')':
    default:
      *success = false;
      return 0;
    }
  }
}

word_t expr(char *e, bool *success)
{
  if (!make_token(e))
  {
    *success = false;
    return 0;
  }

  /* TODO: Insert codes to evaluate the expression. */
  word_t ret = eval(0, nr_token - 1, success);
  return *success ? ret : 0;
}
