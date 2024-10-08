/**
 * LR(1) expression parser via recursive ascent
 *
 * @author Tobias Weber
 * @date 21-aug-2022
 * @license see 'LICENSE' file
 *
 * Reference for the algorithm:
 *    https://doi.org/10.1016/0020-0190(88)90061-0
 */

#ifndef __EXPR_PARSER_RECASC_H__
#define __EXPR_PARSER_RECASC_H__


#include <vector>
#include <stack>
#include <memory>
#include <variant>
#include <unordered_map>
#include <string>


template<class T = double>
class ExprParser
{
public:
	using t_val = T;
	using t_sym = std::variant<t_val, std::string>;

	struct Token
	{
		enum : int
		{
			SCALAR  = 1000,
			IDENT   = 1001,
			END     = 1002,

			INVALID = 10000,
		};

		int id{INVALID};
		t_val val{};
		std::string strval{};
	};


public:
	ExprParser();
	~ExprParser();
	ExprParser(const ExprParser<T>&);
	ExprParser& operator=(const ExprParser<T>&);

	t_val Parse(const std::string& expr);


protected:
	// --------------------------------------------------------------------
	// lexer
	// --------------------------------------------------------------------
	// find all matching tokens for input string
	static std::vector<Token> get_matching_tokens(const std::string& str);

	// returns the next token
	static Token lex(std::istream*);

	void GetNextLookahead();
	// --------------------------------------------------------------------

	// --------------------------------------------------------------------
	t_val GetValue(const t_sym& sym) const;
	t_val GetIdentValue(const std::string& ident) const;
	t_sym AssignVar(const std::string& ident, const t_sym& arg);
	t_sym CallFunc(const std::string& ident) const;
	t_sym CallFunc(const std::string& ident, const t_sym& arg) const;
	t_sym CallFunc(const std::string& ident, const t_sym& arg1, const t_sym& arg2) const;
	// --------------------------------------------------------------------

	// --------------------------------------------------------------------
	// LR closures
	// --------------------------------------------------------------------
	void start();
	void after_expr();

	void uadd_after_op(int op='+');
	void after_uadd(int op='+');

	void add_after_op(int op='+');
	void after_add(int op='+');

	void mul_after_op(int op='*');
	void after_mul(int op='*');

	void pow_after_op();
	void after_pow();

	void after_bracket();
	void bracket_after_expr();
	void after_bracket_expr();

	void after_ident();
	void after_scalar();

	void assign_after_ident();
	void after_assign();

	void funccall_after_ident();
	void funccall_after_arg();
	void funccall_after_comma();
	void funccall_after_arg2();
	void after_funccall_0args();
	void after_funccall_1arg();
	void after_funccall_2args();
	// --------------------------------------------------------------------

	void TransitionError(const char* func, int token);


private:
	std::string m_toparse{};
	std::shared_ptr<std::istream> m_istr{};

	Token m_lookahead{};
	std::stack<t_sym> m_symbols{};
	bool m_accepted{false};

	std::size_t m_dist_to_jump{0};

	// --------------------------------------------------------------------
	// Tables
	// --------------------------------------------------------------------
	// symbol table
	std::unordered_map<std::string, t_val> m_mapSymbols{};

	// zero-args function table
	std::unordered_map<std::string, t_val(*)()> m_mapFuncs0{};

	// one-arg function table
	std::unordered_map<std::string, t_val(*)(t_val)> m_mapFuncs1{};

	// two-args function table
	std::unordered_map<std::string, t_val(*)(t_val, t_val)> m_mapFuncs2{};
	// ----------------------------------------------------------------------------
};

#endif
