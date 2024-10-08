/**
 * LR(1) expression parser via recursive ascent
 *
 * @author Tobias Weber
 * @date 21-aug-2022
 * @license see 'LICENSE' file
 *
 * Reference for the algorithm:
 *   https://doi.org/10.1016/0020-0190(88)90061-0
 */

#include "ExprParser.h"

#include <exception>
#include <iostream>
#include <fstream>
#include <sstream>
#include <random>
#include <limits>
#include <regex>
#include <cmath>


static std::mt19937 g_rng{std::random_device{}()};


template<class T>
ExprParser<T>::ExprParser() :
	m_mapSymbols  // symbol table
	{
		{ "pi", t_val(M_PI) },
	},

	m_mapFuncs0   // zero-args function table
	{
		{ "rand", []() -> t_val
			{
				if constexpr(std::is_floating_point_v<t_val>)
					return (t_val)std::uniform_real_distribution<t_val>(
						t_val(0), t_val(1))(g_rng);
				else if constexpr(std::is_integral_v<t_val>)
					return (t_val)std::uniform_int_distribution<t_val>(
						std::numeric_limits<t_val>::min(),
						std::numeric_limits<t_val>::max())(g_rng);
				else
					throw std::invalid_argument("Invalid number type.");
			} },
	},

	m_mapFuncs1   // one-arg function table
	{
		{ "sin", [](t_val x) -> t_val { return (t_val)std::sin(x); } },
		{ "cos", [](t_val x) -> t_val { return (t_val)std::cos(x); } },
		{ "tan", [](t_val x) -> t_val { return (t_val)std::tan(x); } },
		{ "asin", [](t_val x) -> t_val { return (t_val)std::asin(x); } },
		{ "acos", [](t_val x) -> t_val { return (t_val)std::acos(x); } },
		{ "atan", [](t_val x) -> t_val { return (t_val)std::atan(x); } },

		{ "sinh", [](t_val x) -> t_val { return (t_val)std::sin(x); } },
		{ "cosh", [](t_val x) -> t_val { return (t_val)std::cos(x); } },
		{ "tanh", [](t_val x) -> t_val { return (t_val)std::tan(x); } },
		{ "asinh", [](t_val x) -> t_val { return (t_val)std::asin(x); } },
		{ "acosh", [](t_val x) -> t_val { return (t_val)std::acos(x); } },
		{ "atanh", [](t_val x) -> t_val { return (t_val)std::atan(x); } },

		{ "sqrt", [](t_val x) -> t_val { return (t_val)std::sqrt(x); } },
		{ "cbrt", [](t_val x) -> t_val { return (t_val)std::cbrt(x); } },

		{ "exp", [](t_val x) -> t_val { return (t_val)std::exp(x); } },
		{ "log", [](t_val x) -> t_val { return (t_val)std::log(x); } },
		{ "log10", [](t_val x) -> t_val { return (t_val)std::log10(x); } },
		{ "log2", [](t_val x) -> t_val { return (t_val)std::log2(x); } },

		{ "round", [](t_val x) -> t_val { return (t_val)std::round(x); } },
		{ "ceil", [](t_val x) -> t_val { return (t_val)std::ceil(x); } },
		{ "floor", [](t_val x) -> t_val { return (t_val)std::floor(x); } },
		{ "abs", [](t_val x) -> t_val { return (t_val)std::abs(x); } },

		{ "erf", [](t_val x) -> t_val { return (t_val)std::erf(x); } },
		{ "erfc", [](t_val x) -> t_val { return (t_val)std::erfc(x); } },
	},

	m_mapFuncs2   // two-args function table
	{
		{ "pow", [](t_val x, t_val y) -> t_val { return (t_val)std::pow(x, y); } },
		{ "atan2", [](t_val y, t_val x) -> t_val { return (t_val)std::atan2(y, x); } },

		{ "rand", [](t_val min, t_val max) -> t_val
			{
				if constexpr(std::is_floating_point_v<t_val>)
					return (t_val)std::uniform_real_distribution<t_val>(
						min, max)(g_rng);
				else if constexpr(std::is_integral_v<t_val>)
					return (t_val)std::uniform_int_distribution<t_val>(
						min, max)(g_rng);
				else
					throw std::invalid_argument("Invalid number type.");
			} },

		{ "mod", [](t_val x, t_val y) -> t_val
			{
				if constexpr(std::is_floating_point_v<t_val>)
					return (t_val)std::fmod(x, y);
				else if constexpr(std::is_integral_v<t_val>)
					return (t_val)(x % y);
				else
					throw std::invalid_argument("Invalid number type.");
			} },
	}
{ }


template<class T>
ExprParser<T>::~ExprParser()
{ }


template<class T>
ExprParser<T>::ExprParser(const ExprParser<T>& parser)
{
	this->operator=(parser);
}


template<class T>
ExprParser<T>& ExprParser<T>::operator=(const ExprParser<T>& parser)
{
	this->m_istr = parser.m_istr;

	this->m_lookahead = parser.m_lookahead;
	this->m_symbols = parser.m_symbols;
	this->m_accepted = parser.m_accepted;
	this->m_dist_to_jump = parser.m_dist_to_jump;

	this->m_mapSymbols = parser.m_mapSymbols;
	this->m_mapFuncs0 = parser.m_mapFuncs0;
	this->m_mapFuncs1 = parser.m_mapFuncs1;
	this->m_mapFuncs2 = parser.m_mapFuncs2;

	return *this;
}


/**
 * find all matching tokens for input string
 */
template<class T>
std::vector<typename ExprParser<T>::Token>
ExprParser<T>::get_matching_tokens(const std::string& str)
{
	std::vector<Token> matches;

	if constexpr(std::is_floating_point_v<t_val>)
	{       // real
		std::regex regex{"[0-9]+(\\.[0-9]*)?([Ee][+-]?[0-9]*)?"};
		std::smatch smatch;
		if(std::regex_match(str, smatch, regex))
		{
			Token tok{};
			tok.id = Token::SCALAR;
			std::istringstream{str} >> tok.val;
			matches.emplace_back(std::move(tok));
		}
	}
	else if constexpr(std::is_integral_v<t_val>)
	{       // int
		std::regex regex{"[0-9]+"};
		std::smatch smatch;
		if(std::regex_match(str, smatch, regex))
		{
			Token tok{};
			tok.id = Token::SCALAR;
			std::istringstream{str} >> tok.val;
			matches.emplace_back(std::move(tok));
		}
	}
	else
	{
		throw std::invalid_argument("Invalid number type.");
	}

	// ident
	std::regex regex{"[A-Za-z]+[A-Za-z0-9]*"};
	std::smatch smatch;
	if(std::regex_match(str, smatch, regex))
	{
		Token tok{};
		tok.id = Token::IDENT;
		tok.strval = str;
		matches.emplace_back(std::move(tok));
	}

	// tokens represented by themselves
	if(str == "+" || str == "-" || str == "*" || str == "/" || str == "%" ||
		str == "^" || str == "(" || str == ")" || str == "," || str == "=")
	{
		Token tok{};
		tok.id = (int)str[0];
		matches.emplace_back(std::move(tok));
	}

	return matches;
}


/**
 * @return [token, yylval, yytext]
 */
template<class T>
typename ExprParser<T>::Token ExprParser<T>::lex(std::istream* istr)
{
	std::string input, longest_input;
	std::vector<Token> longest_matching;

	// find longest matching token
	while(1)
	{
		char c = istr->get();

		if(istr->eof())
			break;
		// if outside any other match...
		if(longest_matching.size() == 0)
		{
			// ...ignore white spaces
			if(c==' ' || c=='\t')
				continue;
			// ...end on new line
			if(c=='\n')
			{
				Token tok{};
				tok.id = Token::END;
				tok.strval = longest_input;
				return tok;
			}
		}

		input += c;
		auto matching = get_matching_tokens(input);
		if(matching.size())
		{
			longest_input = input;
			longest_matching = std::move(matching);

			if(istr->peek() == std::char_traits<char>::eof() || istr->eof())
				break;
		}
		else
		{
			// no more matches
			istr->putback(c);
			break;
		}
	}

	// at EOF
	if(longest_matching.size() == 0 && input.length() == 0)
	{
		Token tok{};
		tok.id = Token::END;
		tok.strval = longest_input;
		return tok;
	}

	// nothing matches
	if(longest_matching.size() == 0)
	{
		std::cerr << "Invalid input in lexer: \"" << input << "\"." << std::endl;

		Token tok{};
		tok.id = Token::INVALID;
		tok.strval = longest_input;
		return tok;
	}

	// several possible matches
	if(longest_matching.size() > 1)
	{
		std::cerr << "Warning: Ambiguous match in lexer for token \"" << longest_input << "\"." << std::endl;
	}

	// found match
	if(longest_matching.size())
	{
		return longest_matching[0];
	}

	// should not get here
	Token tok{};
	tok.id = Token::INVALID;
	tok.strval = longest_input;
	return tok;
}


template<class T>
void ExprParser<T>::GetNextLookahead()
{
	m_lookahead = lex(m_istr.get());
}


/**
 * get the value of a symbol
 */
template<class T>
typename ExprParser<T>::t_val ExprParser<T>::GetValue(
	const typename ExprParser<T>::t_sym& sym) const
{
	// t_val constant
	if(std::holds_alternative<t_val>(sym))
		return std::get<t_val>(sym);

	// string naming a variable
	else if(std::holds_alternative<std::string>(sym))
		return GetIdentValue(std::get<std::string>(sym));

	return t_val{};
}


/**
 * get the value of a variable with given id
 */
template<class T>
typename ExprParser<T>::t_val ExprParser<T>::GetIdentValue(const std::string& id) const
{
	if(auto iter = m_mapSymbols.find(id); iter != m_mapSymbols.end())
		return iter->second;

	throw std::runtime_error("Unknown variable \"" + id + "\".");
}


template<class T>
void ExprParser<T>::TransitionError(const char* func, int token)
{
	std::ostringstream ostr;
	ostr << "No transition from " << func
		<< " and look-ahead terminal " << token;
	if(token == Token::SCALAR)
		ostr << " (scalar)";
	else if(token == Token::IDENT)
		ostr << " (ident)";
	else if(token == Token::END)
		ostr << " (end)";
	else if(std::isprint(token))
		ostr << " ('" << char(token) << "')";
	ostr << ".";

	ostr << " Input expression: \"" << m_toparse << "\".";

	throw std::runtime_error(ostr.str());
}


/**
 * assign a variable
 */
template<class T>
typename ExprParser<T>::t_sym ExprParser<T>::AssignVar(
	const std::string& id,
	const typename ExprParser<T>::t_sym& arg)
{
	// does the variable already exist?
	if(auto iter = m_mapSymbols.find(id); iter != m_mapSymbols.end())
	{
		iter->second = GetValue(arg);
		return iter->second;
	}

	// otherwise insert variable
	else
	{
		iter = m_mapSymbols.insert(std::make_pair(id, GetValue(arg))).first;
		return iter->second;
	}
}


/**
 * call a function with 0 arguments
 */
template<class T>
typename ExprParser<T>::t_sym ExprParser<T>::CallFunc(const std::string& id) const
{
	if(auto iter = m_mapFuncs0.find(id); iter != m_mapFuncs0.end())
		return (*iter->second)();

	throw std::runtime_error("Unknown function \"" + id + "\".");
}


/**
 * call a function with 1 argument
 */
template<class T>
typename ExprParser<T>::t_sym ExprParser<T>::CallFunc(
	const std::string& id,
	const typename ExprParser<T>::t_sym& arg) const
{
	if(auto iter = m_mapFuncs1.find(id); iter != m_mapFuncs1.end())
		return (*iter->second)(GetValue(arg));

	throw std::runtime_error("Unknown function \"" + id + "\".");
}


/**
 * call a function with 2 arguments
 */
template<class T>
typename ExprParser<T>::t_sym ExprParser<T>::CallFunc(
	const std::string& id,
	const typename ExprParser<T>::t_sym& arg1,
	const typename ExprParser<T>::t_sym& arg2) const
{
	if(auto iter = m_mapFuncs2.find(id); iter != m_mapFuncs2.end())
	{
		t_val retval = (*iter->second)(GetValue(arg1), GetValue(arg2));
		return t_sym{retval};
	}

	throw std::runtime_error("Unknown function \"" + id + "\".");
}


template<class T>
typename ExprParser<T>::t_val ExprParser<T>::Parse(const std::string& expr)
{
	m_toparse = expr;
	m_istr = std::make_shared<std::istringstream>(expr);
	m_lookahead = Token{};
	m_dist_to_jump = 0;
	m_accepted = false;
	while(!m_symbols.empty())
		m_symbols.pop();

	GetNextLookahead();
	start();

	if(m_symbols.size() && m_accepted)
		return GetValue(m_symbols.top());

	return t_val{};  // error
}


/**
 * start -> •expr ｜ end
 */
template<class T>
void ExprParser<T>::start()
{
	int lookahead = m_lookahead.id;
	switch(lookahead)
	{
		case '+': case '-':
		{
			GetNextLookahead();
			uadd_after_op(lookahead);
			break;
		}
		case '(':
		{
			GetNextLookahead();
			after_bracket();
			break;
		}
		case Token::SCALAR:
		{
			m_symbols.emplace(m_lookahead.val);
			GetNextLookahead();
			after_scalar();
			break;
		}
		case Token::IDENT:
		{
			m_symbols.emplace(m_lookahead.strval);
			GetNextLookahead();
			after_ident();
			break;
		}
		default:
		{
			TransitionError(__FUNCTION__, m_lookahead.id);
			break;
		}
	}

	while(!m_dist_to_jump && m_symbols.size() && !m_accepted)
		after_expr();

	if(m_dist_to_jump > 0)
		--m_dist_to_jump;
}


/**
 * start -> expr•
 */
template<class T>
void ExprParser<T>::after_expr()
{
	int lookahead = m_lookahead.id;
	switch(lookahead)
	{
		case '+': case '-':
		{
			GetNextLookahead();
			add_after_op(lookahead);
			break;
		}
		case '*': case '/': case '%':
		{
			GetNextLookahead();
			mul_after_op(lookahead);
			break;
		}
		case '^':
		{
			GetNextLookahead();
			pow_after_op();
			break;
		}
		case Token::END:
		{
			m_accepted = true;
			break;
		}
		default:
		{
			TransitionError(__FUNCTION__, m_lookahead.id);
			break;
		}
	}

	if(m_dist_to_jump > 0)
		--m_dist_to_jump;
}


/**
 * expr -> expr + •expr
 */
template<class T>
void ExprParser<T>::add_after_op(int op)
{
	int lookahead = m_lookahead.id;
	switch(lookahead)
	{
		case '+': case '-':
		{
			GetNextLookahead();
			uadd_after_op(lookahead);
			break;
		}
		case '(':
		{
			GetNextLookahead();
			after_bracket();
			break;
		}
		case Token::SCALAR:
		{
			m_symbols.emplace(m_lookahead.val);
			GetNextLookahead();
			after_scalar();
			break;
		}
		case Token::IDENT:
		{
			m_symbols.emplace(m_lookahead.strval);
			GetNextLookahead();
			after_ident();
			break;
		}
		default:
		{
			TransitionError(__FUNCTION__, m_lookahead.id);
			break;
		}
	}

	while(!m_dist_to_jump && m_symbols.size() && !m_accepted)
		after_add(op);

	if(m_dist_to_jump > 0)
		--m_dist_to_jump;
}


/**
 * expr -> expr + expr•
 */
template<class T>
void ExprParser<T>::after_add(int op)
{
	int lookahead = m_lookahead.id;
	switch(lookahead)
	{
		case '*': case '/': case '%':
		{
			GetNextLookahead();
			mul_after_op(lookahead);
			break;
		}
		case '^':
		{
			GetNextLookahead();
			pow_after_op();
			break;
		}
		case '+': case '-': case ')': case ',': case Token::END:
		{
			m_dist_to_jump = 3;
			t_sym arg1 = std::move(m_symbols.top());
			m_symbols.pop();
			t_sym arg0 = std::move(m_symbols.top());
			m_symbols.pop();

			switch(op)
			{
				case '+':
					// semantic rule: expr -> expr + expr.
					m_symbols.emplace(GetValue(arg0) + GetValue(arg1));
					break;
				case '-':
					// semantic rule: expr -> expr - expr.
					m_symbols.emplace(GetValue(arg0) - GetValue(arg1));
					break;
			}
			break;
		}
		default:
		{
			TransitionError(__FUNCTION__, m_lookahead.id);
			break;
		}
	}

	if(m_dist_to_jump > 0)
		--m_dist_to_jump;
}


/**
 * expr -> expr * •expr
 */
template<class T>
void ExprParser<T>::mul_after_op(int op)
{
	int lookahead = m_lookahead.id;
	switch(lookahead)
	{
		case '+': case '-':
		{
			GetNextLookahead();
			uadd_after_op(lookahead);
			break;
		}
		case '(':
		{
			GetNextLookahead();
			after_bracket();
			break;
		}
		case Token::SCALAR:
		{
			m_symbols.emplace(m_lookahead.val);
			GetNextLookahead();
			after_scalar();
			break;
		}
		case Token::IDENT:
		{
			m_symbols.emplace(m_lookahead.strval);
			GetNextLookahead();
			after_ident();
			break;
		}
		default:
		{
			TransitionError(__FUNCTION__, m_lookahead.id);
			break;
		}
	}

	while(!m_dist_to_jump && m_symbols.size() && !m_accepted)
		after_mul(op);

	if(m_dist_to_jump > 0)
		--m_dist_to_jump;
}


/**
 * expr -> expr * expr•
 */
template<class T>
void ExprParser<T>::after_mul(int op)
{
	switch(m_lookahead.id)
	{
		case '^':
		{
			GetNextLookahead();
			pow_after_op();
			break;
		}
		case '+': case '-': case '*': case '/':
		case '%': case ')': case ',': case Token::END:
		{
			m_dist_to_jump = 3;
			t_sym arg1 = std::move(m_symbols.top());
			m_symbols.pop();
			t_sym arg0 = std::move(m_symbols.top());
			m_symbols.pop();

			switch(op)
			{
				case '*':
					// semantic rule: expr -> expr * expr.
					m_symbols.emplace(GetValue(arg0) * GetValue(arg1));
					break;
				case '/':
					// semantic rule: expr -> expr / expr.
					m_symbols.emplace(GetValue(arg0) / GetValue(arg1));
					break;
				case '%':
					// semantic rule: expr -> expr % expr.
					if constexpr(std::is_floating_point_v<t_val>)
						m_symbols.emplace((t_val)std::fmod(GetValue(arg0), GetValue(arg1)));
					else if constexpr(std::is_integral_v<t_val>)
						m_symbols.emplace((t_val)(GetValue(arg0) % GetValue(arg1)));
					else
						throw std::invalid_argument("Invalid number type.");
				break;
			}
			break;
		}
		default:
		{
			TransitionError(__FUNCTION__, m_lookahead.id);
			break;
		}
	}

	if(m_dist_to_jump > 0)
		--m_dist_to_jump;
}


/**
 * expr -> expr ^ •expr
 */
template<class T>
void ExprParser<T>::pow_after_op()
{
	int lookahead = m_lookahead.id;
	switch(lookahead)
	{
		case '+': case '-':
		{
			GetNextLookahead();
			uadd_after_op(lookahead);
			break;
		}
		case '(':
		{
			GetNextLookahead();
			after_bracket();
			break;
		}
		case Token::SCALAR:
		{
			m_symbols.emplace(m_lookahead.val);
			GetNextLookahead();
			after_scalar();
			break;
		}
		case Token::IDENT:
		{
			m_symbols.emplace(m_lookahead.strval);
			GetNextLookahead();
			after_ident();
			break;
		}
		default:
		{
			TransitionError(__FUNCTION__, m_lookahead.id);
			break;
		}
	}

	while(!m_dist_to_jump && m_symbols.size() && !m_accepted)
		after_pow();

	if(m_dist_to_jump > 0)
		--m_dist_to_jump;
}


/**
 * expr -> expr ^ expr•
 */
template<class T>
void ExprParser<T>::after_pow()
{
	switch(m_lookahead.id)
	{
		case '^':
		{
			GetNextLookahead();
			pow_after_op();
			break;
		}

		case '+': case '-': case '*': case '/':
		case '%': case ',': case ')': case Token::END:
		{
			m_dist_to_jump = 3;
			t_sym arg1 = std::move(m_symbols.top());
			m_symbols.pop();
			t_sym arg0 = std::move(m_symbols.top());
			m_symbols.pop();

			// semantic rule: expr -> expr ^ expr.
			m_symbols.emplace((t_val)std::pow(GetValue(arg0), GetValue(arg1)));
			break;
		}
		default:
		{
			TransitionError(__FUNCTION__, m_lookahead.id);
			break;
		}
	}

	if(m_dist_to_jump > 0)
		--m_dist_to_jump;
}


/**
 * expr -> ( •expr )
 */
template<class T>
void ExprParser<T>::after_bracket()
{
	int lookahead = m_lookahead.id;
	switch(lookahead)
	{
		case '+': case '-':
		{
			GetNextLookahead();
			uadd_after_op(lookahead);
			break;
		}
		case '(':
		{
			GetNextLookahead();
			after_bracket();
			break;
		}
		case Token::SCALAR:
		{
			m_symbols.emplace(m_lookahead.val);
			GetNextLookahead();
			after_scalar();
			break;
		}
		case Token::IDENT:
		{
			m_symbols.emplace(m_lookahead.strval);
			GetNextLookahead();
			after_ident();
			break;
		}
		default:
		{
			TransitionError(__FUNCTION__, m_lookahead.id);
			break;
		}
	}

	while(!m_dist_to_jump && m_symbols.size() && !m_accepted)
		bracket_after_expr();

	if(m_dist_to_jump > 0)
		--m_dist_to_jump;
}


/**
 * expr -> ( expr •)
 */
template<class T>
void ExprParser<T>::bracket_after_expr()
{
	int lookahead = m_lookahead.id;
	switch(lookahead)
	{
		case '+': case '-':
		{
			GetNextLookahead();
			add_after_op(lookahead);
			break;
		}
		case '*': case '/': case '%':
		{
			GetNextLookahead();
			mul_after_op(lookahead);
			break;
		}
		case '^':
		{
			GetNextLookahead();
			pow_after_op();
			break;
		}
		case ')':
		{
			GetNextLookahead();
			after_bracket_expr();
			break;
		}
		default:
		{
			TransitionError(__FUNCTION__, m_lookahead.id);
			break;
		}
	}

	if(m_dist_to_jump > 0)
		--m_dist_to_jump;
}


/**
 * expr -> ident •( )
 */
template<class T>
void ExprParser<T>::after_ident()
{
	switch(m_lookahead.id)
	{
		case '=':
		{
			GetNextLookahead();
			assign_after_ident();
			break;
		}
		case '(':
		{
			GetNextLookahead();
			funccall_after_ident();
			break;
		}
		case '+': case '-': case '*': case '/':
		case '%': case '^': case ',': case ')':
		case Token::END:
		{
			m_dist_to_jump = 1;
			t_sym arg = std::move(m_symbols.top());
			m_symbols.pop();

			// semantic rule: expr -> ident.
			m_symbols.emplace(GetValue(arg));
			break;
		}
		default:
		{
			TransitionError(__FUNCTION__, m_lookahead.id);
			break;
		}
	}

	if(m_dist_to_jump > 0)
		--m_dist_to_jump;
}


/**
 * expr -> ( expr )•
 */
template<class T>
void ExprParser<T>::after_bracket_expr()
{
	switch(m_lookahead.id)
	{
		case '+': case '-': case '*': case '/':
		case '%': case '^': case ',': case ')':
		case Token::END:
		{
			m_dist_to_jump = 3;
			t_sym arg = std::move(m_symbols.top());
			m_symbols.pop();

			// semantic rule: expr -> ( expr ).
			m_symbols.emplace(GetValue(arg));
			break;
		}
		default:
		{
			TransitionError(__FUNCTION__, m_lookahead.id);
			break;
		}
	}

	if(m_dist_to_jump > 0)
		--m_dist_to_jump;
}


/**
 * expr -> ident = •expr
 */
template<class T>
void ExprParser<T>::assign_after_ident()
{
	int lookahead = m_lookahead.id;
	switch(lookahead)
	{
		case '+': case '-':
		{
			GetNextLookahead();
			uadd_after_op(lookahead);
			break;
		}
		case '(':
		{
			GetNextLookahead();
			after_bracket();
			break;
		}
		case Token::SCALAR:
		{
			m_symbols.emplace(m_lookahead.val);
			GetNextLookahead();
			after_scalar();
			break;
		}
		case Token::IDENT:
		{
			m_symbols.emplace(m_lookahead.strval);
			GetNextLookahead();
			after_ident();
			break;
		}
		default:
		{
			TransitionError(__FUNCTION__, m_lookahead.id);
			break;
		}
	}

	while(!m_dist_to_jump && m_symbols.size() && !m_accepted)
		after_assign();

	if(m_dist_to_jump > 0)
		--m_dist_to_jump;
}


/**
 * expr -> ident = expr•
 */
template<class T>
void ExprParser<T>::after_assign()
{
	int lookahead = m_lookahead.id;
	switch(lookahead)
	{
		case '+': case '-':
		{
			GetNextLookahead();
			add_after_op(lookahead);
			break;
		}
		case '*': case '/': case '%':
		{
			GetNextLookahead();
			mul_after_op(lookahead);
			break;
		}
		case '^':
		{
			GetNextLookahead();
			pow_after_op();
			break;
		}
		case ',': case ')': case Token::END:
		{
			m_dist_to_jump = 3;
			t_sym rhs = std::move(m_symbols.top());
			m_symbols.pop();
			t_sym lhs = std::move(m_symbols.top());
			m_symbols.pop();

			// semantic rule: expr -> ident = expr.
			if(std::holds_alternative<std::string>(lhs))
				m_symbols.emplace(AssignVar(std::get<std::string>(lhs), rhs));
			else
				throw std::runtime_error("Assignment needs a variable identifier.");
			break;
		}
		default:
		{
			TransitionError(__FUNCTION__, m_lookahead.id);
			break;
		}
	}

	if(m_dist_to_jump > 0)
		--m_dist_to_jump;
}


/**
 * expr -> ident ( •)
 */
template<class T>
void ExprParser<T>::funccall_after_ident()
{
	int lookahead = m_lookahead.id;
	switch(lookahead)
	{
		case '+': case '-':
		{
			GetNextLookahead();
			uadd_after_op(lookahead);
			break;
		}
		case '(':
		{
			GetNextLookahead();
			after_bracket();
			break;
		}
		case ')':
		{
			GetNextLookahead();
			after_funccall_0args();
			break;
		}
		case Token::SCALAR:
		{
			m_symbols.emplace(m_lookahead.val);
			GetNextLookahead();
			after_scalar();
			break;
		}
		case Token::IDENT:
		{
			m_symbols.emplace(m_lookahead.strval);
			GetNextLookahead();
			after_ident();
			break;
		}
		default:
		{
			TransitionError(__FUNCTION__, m_lookahead.id);
			break;
		}
	}

	while(!m_dist_to_jump && m_symbols.size() && !m_accepted)
		funccall_after_arg();

	if(m_dist_to_jump > 0)
		--m_dist_to_jump;
}


/**
 * expr -> ident ( )•
 */
template<class T>
void ExprParser<T>::after_funccall_0args()
{
	switch(m_lookahead.id)
	{
		case '+': case '-': case '*': case '/':
		case '%': case '^': case ',': case ')':
		case Token::END:
		{
			m_dist_to_jump = 3;
			t_sym arg = std::move(m_symbols.top());
			m_symbols.pop();

			// semantic rule: expr -> ident ( ).
			if(std::holds_alternative<std::string>(arg))
				m_symbols.emplace(CallFunc(std::get<std::string>(arg)));
			else
				throw std::runtime_error("Function call needs an identifier.");
			break;
		}
		default:
		{
			TransitionError(__FUNCTION__, m_lookahead.id);
			break;
		}
	}

	if(m_dist_to_jump > 0)
		--m_dist_to_jump;
}


/**
 * expr -> ident ( expr •)
 */
template<class T>
void ExprParser<T>::funccall_after_arg()
{
	int lookahead = m_lookahead.id;
	switch(lookahead)
	{
		case '+': case '-':
		{
			GetNextLookahead();
			add_after_op(lookahead);
			break;
		}
		case '*': case '/': case '%':
		{
			GetNextLookahead();
			mul_after_op(lookahead);
			break;
		}
		case '^':
		{
			GetNextLookahead();
			pow_after_op();
			break;
		}
		case ',':
		{
			GetNextLookahead();
			funccall_after_comma();
			break;
		}
		case ')':
		{
			GetNextLookahead();
			after_funccall_1arg();
			break;
		}
		default:
		{
			TransitionError(__FUNCTION__, m_lookahead.id);
			break;
		}
	}

	if(m_dist_to_jump > 0)
		--m_dist_to_jump;
}


/**
 * expr -> ident ( expr )•
 */
template<class T>
void ExprParser<T>::after_funccall_1arg()
{
	switch(m_lookahead.id)
	{
		case '+': case '-': case '*': case '/':
		case '%': case '^': case ',': case ')':
		case Token::END:
		{
			m_dist_to_jump = 4;
			t_sym arg1 = std::move(m_symbols.top());
			m_symbols.pop();
			t_sym arg0 = std::move(m_symbols.top());
			m_symbols.pop();

			// semantic rule: expr -> ident ( expr ).
			if(std::holds_alternative<std::string>(arg0))
				m_symbols.emplace(CallFunc(std::get<std::string>(arg0), arg1));
			else
				throw std::runtime_error("Function call needs an identifier.");
			break;
		}
		default:
		{
			TransitionError(__FUNCTION__, m_lookahead.id);
			break;
		}
	}

	if(m_dist_to_jump > 0)
		--m_dist_to_jump;
}


/**
 * expr -> ident ( expr , •expr )
 */
template<class T>
void ExprParser<T>::funccall_after_comma()
{
	int lookahead = m_lookahead.id;
	switch(lookahead)
	{
		case '+': case '-':
		{
			GetNextLookahead();
			uadd_after_op(lookahead);
			break;
		}
		case '(':
		{
			GetNextLookahead();
			after_bracket();
			break;
		}
		case Token::SCALAR:
		{
			m_symbols.emplace(m_lookahead.val);
			GetNextLookahead();
			after_scalar();
			break;
		}
		case Token::IDENT:
		{
			m_symbols.emplace(m_lookahead.strval);
			GetNextLookahead();
			after_ident();
			break;
		}
		default:
		{
			TransitionError(__FUNCTION__, m_lookahead.id);
			break;
		}
	}

	while(!m_dist_to_jump && m_symbols.size() && !m_accepted)
		funccall_after_arg2();

	if(m_dist_to_jump > 0)
		--m_dist_to_jump;
}


/**
 * expr -> ident ( expr , expr •)
 */
template<class T>
void ExprParser<T>::funccall_after_arg2()
{
	int lookahead = m_lookahead.id;
	switch(lookahead)
	{
		case '+': case '-':
		{
			GetNextLookahead();
			add_after_op(lookahead);
			break;
		}
		case '*': case '/': case '%':
		{
			GetNextLookahead();
			mul_after_op(lookahead);
			break;
		}
		case '^':
		{
			GetNextLookahead();
			pow_after_op();
			break;
		}
		case ')':
		{
			GetNextLookahead();
			after_funccall_2args();
			break;
		}
		default:
		{
			TransitionError(__FUNCTION__, m_lookahead.id);
			break;
		}
	}

	if(m_dist_to_jump > 0)
		--m_dist_to_jump;
}


/**
 * expr -> scalar•
 */
template<class T>
void ExprParser<T>::after_scalar()
{
	switch(m_lookahead.id)
	{
		case '+': case '-': case '*': case '/':
		case '%': case '^': case ',': case ')':
		case Token::END:
		{
			m_dist_to_jump = 1;
			t_sym arg = std::move(m_symbols.top());
			m_symbols.pop();

			// semantic rule: expr -> scalar.
			m_symbols.emplace(GetValue(arg));
			break;
		}
		default:
		{
			TransitionError(__FUNCTION__, m_lookahead.id);
			break;
		}
	}

	if(m_dist_to_jump > 0)
		--m_dist_to_jump;
}


/**
 * expr -> ident ( expr , expr )•
 */
template<class T>
void ExprParser<T>::after_funccall_2args()
{
	switch(m_lookahead.id)
	{
		case '+': case '-': case '*': case '/':
		case '%': case '^': case ',': case ')':
		case Token::END:
		{
			m_dist_to_jump = 6;
			t_sym arg2 = std::move(m_symbols.top());
			m_symbols.pop();
			t_sym arg1 = std::move(m_symbols.top());
			m_symbols.pop();
			t_sym arg0 = std::move(m_symbols.top());
			m_symbols.pop();

			// semantic rule: expr -> ident ( expr , expr ).
			if(std::holds_alternative<std::string>(arg0))
				m_symbols.emplace(CallFunc(std::get<std::string>(arg0), arg1, arg2));
			else
				throw std::runtime_error("Function call needs an identifier.");
			break;
		}
		default:
		{
			TransitionError(__FUNCTION__, m_lookahead.id);
			break;
		}
	}

	if(m_dist_to_jump > 0)
		--m_dist_to_jump;
}


/**
 * expr -> + •expr
 */
template<class T>
void ExprParser<T>::uadd_after_op(int op)
{
	int lookahead = m_lookahead.id;
	switch(lookahead)
	{
		case '+': case '-':
		{
			GetNextLookahead();
			uadd_after_op(lookahead);
			break;
		}
		case '(':
		{
			GetNextLookahead();
			after_bracket();
			break;
		}
		case Token::SCALAR:
		{
			m_symbols.emplace(m_lookahead.val);
			GetNextLookahead();
			after_scalar();
			break;
		}
		case Token::IDENT:
		{
			m_symbols.emplace(m_lookahead.strval);
			GetNextLookahead();
			after_ident();
			break;
		}
		default:
		{
			TransitionError(__FUNCTION__, m_lookahead.id);
			break;
		}
	}

	while(!m_dist_to_jump && m_symbols.size() && !m_accepted)
		after_uadd(op);

	if(m_dist_to_jump > 0)
		--m_dist_to_jump;
}


/**
 * expr -> + expr•
 */
template<class T>
void ExprParser<T>::after_uadd(int op)
{
	int lookahead = m_lookahead.id;
	switch(lookahead)
	{
		case '*': case '/': case '%':
		{
			GetNextLookahead();
			mul_after_op(lookahead);
			break;
		}
		case '^':
		{
			GetNextLookahead();
			pow_after_op();
			break;
		}
		case '+': case '-': case ',': case ')': case Token::END:
		{
			m_dist_to_jump = 2;
			t_sym arg = std::move(m_symbols.top());
			m_symbols.pop();

			switch(op)
			{
				case '+':
					// semantic rule: expr -> + expr.
					m_symbols.emplace(GetValue(arg));
					break;
				case '-':
					// semantic rule: expr -> - expr.
					m_symbols.emplace(-GetValue(arg));
					break;
			}
			break;
		}
		default:
		{
			TransitionError(__FUNCTION__, m_lookahead.id);
			break;
		}
	}

	if(m_dist_to_jump > 0)
		--m_dist_to_jump;
}


// --------------------------------------------------------------------------------
// explicit instantiations
template class ExprParser<double>;
template class ExprParser<int>;
// --------------------------------------------------------------------------------
