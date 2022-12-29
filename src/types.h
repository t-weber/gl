/**
 * global type definitions
 * @author Tobias Weber <tweber@ill.fr>
 * @date feb-2021
 * @license GPLv3, see 'LICENSE' file
 */

#ifndef __GLSCENE_TYPES_H__
#define __GLSCENE_TYPES_H__


#include "tlibs2/libs/maths.h"


// program identifier
#define APPL_IDENT "glscene"

// program title
#define APPL_TITLE "Gl Scene"

// version number of this software
#define APPL_VERSION "0.2"

#define FILE_BASENAME "glscene."


/**
 * fixed-size array wrapper
 */
template<class T, std::size_t N>
class t_arr : public std::array<T, N>
{
public:
	using allocator_type = void;


public:
	constexpr t_arr() noexcept : std::array<T, N>{}
	{
		for(std::size_t i=0; i<this->size(); ++i)
			this->operator[](i) = T{};
	}


	/*constexpr*/ ~t_arr() noexcept = default;


	/**
	 * dummy constructor to fulfill interface requirements
	 */
	constexpr t_arr(std::size_t) noexcept : t_arr{}
	{};


	/**
	 * copy constructor
	 */
	constexpr t_arr(const t_arr& other) noexcept
	{
		this->operator=(other);
	}


	/**
	 * move constructor
	 */
	constexpr t_arr(t_arr&& other) noexcept
	{
		this->operator=(std::forward<t_arr&&>(other));
	}


	/**
	 * assignment
	 */
	constexpr t_arr& operator=(const t_arr& other) noexcept
	{
		//static_cast<std::array<T, N>*>(this)->operator=(other);
		std::array<T, N>::operator=(other);
		return *this;
	}


	/**
	 * movement
	 */
	constexpr t_arr& operator=(t_arr&& other) noexcept
	{
		std::array<T, N>::operator=(std::forward<std::array<T,N>&&>(other));
		return *this;
	}
};


template<class T> using t_arr2 = t_arr<T, 2>;
template<class T> using t_arr4 = t_arr<T, 4>;


using t_real = double;
using t_int = int;

// dynamic container types
using t_vec = tl2::vec<t_real, std::vector>;
using t_vec_int = tl2::vec<t_int, std::vector>;
using t_mat = tl2::mat<t_real, std::vector>;

// static container types
using t_vec2 = tl2::vec<t_real, t_arr2>;
using t_vec2_int = tl2::vec<t_int, t_arr2>;
using t_mat22 = tl2::mat<t_real, t_arr4>;


#endif
