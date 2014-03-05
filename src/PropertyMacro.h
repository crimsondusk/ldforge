/*
 *  LDForge: LDraw parts authoring CAD
 *  Copyright (C) 2013, 2014 Santeri Piippo
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#pragma once

// =============================================================================
//
// Identifier names
//
#define PROPERTY_SET_ACCESSOR(NAME)			set##NAME
#define PROPERTY_GET_ACCESSOR(NAME)			get##NAME
#define PROPERTY_IS_ACCESSOR(NAME)			is##NAME // for bool types
#define PROPERTY_MEMBER_NAME(NAME)			m_##NAME

// Names of operations
#define PROPERTY_APPEND_OPERATION(NAME)		appendTo##NAME
#define PROPERTY_PREPEND_OPERATION(NAME)	prependTo##NAME
#define PROPERTY_REPLACE_OPERATION(NAME)	replaceIn##NAME
#define PROPERTY_INCREASE_OPERATION(NAME)	increase##NAME
#define PROPERTY_DECREASE_OPERATION(NAME)	decrease##NAME
#define PROPERTY_TOGGLE_OPERATION(NAME)		toggle##NAME
#define PROPERTY_PUSH_OPERATION(NAME)		pushTo##NAME
#define PROPERTY_REMOVE_OPERATION(NAME)		removeFrom##NAME
#define PROPERTY_CLEAR_OPERATION(NAME)		clear##NAME
#define PROPERTY_COUNT_OPERATION(NAME)		count##NAME

// Operation definitions
// These are the methods of the list type that are called in the operations.
#define PROPERTY_APPEND_METHOD_NAME			append		// QString::append
#define PROPERTY_PREPEND_METHOD_NAME		prepend		// QString::prepend
#define PROPERTY_REPLACE_METHOD_NAME		replace		// QString::replace
#define PROPERTY_PUSH_METHOD_NAME			append		// QList<T>::append
#define PROPERTY_REMOVE_METHOD_NAME			removeOne	// QList<T>::removeOne
#define PROPERTY_CLEAR_METHOD_NAME			clear		// QList<T>::clear

// =============================================================================
//
// Main PROPERTY macro
//
#define PROPERTY(ACCESS, TYPE, NAME, OPS, WRITETYPE)							\
	private:																	\
		TYPE PROPERTY_MEMBER_NAME(NAME);										\
																				\
	public:																		\
		inline TYPE const& PROPERTY_GET_READ_METHOD (NAME, OPS) const			\
		{																		\
			return PROPERTY_MEMBER_NAME(NAME); 									\
		}																		\
																				\
	ACCESS:																		\
		PROPERTY_MAKE_WRITE (TYPE, NAME, WRITETYPE)								\
		PROPERTY_DEFINE_OPERATIONS (TYPE, NAME, OPS)

// =============================================================================
//
// PROPERTY_GET_READ_METHOD
//
// This macro uses the OPS argument to construct the name of the actual
// macro which returns the name of the get accessor. This is so that the
// bool properties get is<NAME>() accessors while non-bools get get<NAME>()
//
#define PROPERTY_GET_READ_METHOD(NAME, OPS)										\
	PROPERTY_GET_READ_METHOD_##OPS (NAME)

#define PROPERTY_GET_READ_METHOD_BOOL_OPS(NAME) PROPERTY_IS_ACCESSOR (NAME)()
#define PROPERTY_GET_READ_METHOD_NO_OPS(NAME) PROPERTY_GET_ACCESSOR (NAME)()
#define PROPERTY_GET_READ_METHOD_STR_OPS(NAME) PROPERTY_GET_ACCESSOR (NAME)()
#define PROPERTY_GET_READ_METHOD_NUM_OPS(NAME) PROPERTY_GET_ACCESSOR (NAME)()
#define PROPERTY_GET_READ_METHOD_LIST_OPS(NAME) PROPERTY_GET_ACCESSOR (NAME)()

// =============================================================================
//
// PROPERTY_MAKE_WRITE
//
// This macro uses the WRITETYPE argument to construct the set accessor of the
// property. If WRITETYPE is STOCK_WRITE, an inline method is defined to just
// set the new value of the property. If WRITETYPE is CUSTOM_WRITE, the accessor
// is merely declared and is left for the user to define.
//
#define PROPERTY_MAKE_WRITE(TYPE, NAME, WRITETYPE)								\
	PROPERTY_MAKE_WRITE_##WRITETYPE (TYPE, NAME)

#define PROPERTY_MAKE_WRITE_STOCK_WRITE(TYPE, NAME)								\
		inline void set##NAME (TYPE const& new##NAME)							\
		{																		\
			PROPERTY_MEMBER_NAME(NAME) = new##NAME;								\
		}

#define PROPERTY_MAKE_WRITE_CUSTOM_WRITE(TYPE, NAME)							\
		void set##NAME (TYPE const& new##NAME);									\

// =============================================================================
//
// PROPERTY_DEFINE_OPERATIONS
//
// This macro may expand into methods defining additional operations for the
// method. 

#define PROPERTY_DEFINE_OPERATIONS(TYPE, NAME, OPS)								\
	DEFINE_PROPERTY_##OPS (TYPE, NAME)

// =============================================================================
//
// DEFINE_PROPERTY_NO_OPS
//
// Obviously NO_OPS expands into no operations.
//
#define DEFINE_PROPERTY_NO_OPS(TYPE, NAME)

// =============================================================================
//
// DEFINE_PROPERTY_STR_OPS
//
#define DEFINE_PROPERTY_STR_OPS(TYPE, NAME)										\
		void PROPERTY_APPEND_OPERATION(NAME) (const TYPE& a)					\
		{																		\
			TYPE tmp (PROPERTY_MEMBER_NAME(NAME));								\
			tmp.PROPERTY_APPEND_METHOD_NAME (a);								\
			set##NAME (tmp);													\
		}																		\
																				\
		void PROPERTY_PREPEND_OPERATION(NAME) (const TYPE& a)					\
		{																		\
			TYPE tmp (PROPERTY_MEMBER_NAME(NAME));								\
			tmp.PROPERTY_PREPEND_METHOD_NAME (a);								\
			set##NAME (tmp);													\
		}																		\
																				\
		void PROPERTY_REPLACE_OPERATION(NAME) (const TYPE& a, const TYPE& b)	\
		{																		\
			TYPE tmp (PROPERTY_MEMBER_NAME(NAME));								\
			tmp.PROPERTY_REPLACE_METHOD_NAME (a, b);							\
			set##NAME (tmp);													\
		}

// =============================================================================
//
// DEFINE_PROPERTY_NUM_OPS
//
#define DEFINE_PROPERTY_NUM_OPS(TYPE, NAME)										\
		inline void PROPERTY_INCREASE_OPERATION(NAME) (TYPE a = 1)				\
		{																		\
			set##NAME (PROPERTY_MEMBER_NAME(NAME) + a);							\
		}																		\
																				\
		inline void PROPERTY_DECREASE_OPERATION(NAME) (TYPE a = 1)				\
		{																		\
			set##NAME (PROPERTY_MEMBER_NAME(NAME) - a);							\
		}

// =============================================================================
//
// DEFINE_PROPERTY_BOOL_OPS
//
#define DEFINE_PROPERTY_BOOL_OPS(TYPE, NAME)									\
		inline void PROPERTY_TOGGLE_OPERATION(NAME)()							\
		{																		\
			set##NAME (!PROPERTY_MEMBER_NAME(NAME));							\
		}

// =============================================================================
//
// DEFINE_PROPERTY_LIST_OPS
//
#define DEFINE_PROPERTY_LIST_OPS(TYPE, NAME)									\
		void PROPERTY_PUSH_OPERATION(NAME) (const TYPE::value_type& a)			\
		{																		\
			PROPERTY_MEMBER_NAME(NAME).PROPERTY_PUSH_METHOD_NAME (a);			\
		}																		\
																				\
		void PROPERTY_REMOVE_OPERATION(NAME) (const TYPE::value_type& a)		\
		{																		\
			PROPERTY_MEMBER_NAME(NAME).PROPERTY_REMOVE_METHOD_NAME (a);			\
		}																		\
																				\
		inline void PROPERTY_CLEAR_OPERATION(NAME)()							\
		{																		\
			PROPERTY_MEMBER_NAME(NAME).PROPERTY_CLEAR_METHOD_NAME();			\
		}																		\
																				\
	public:																		\
		inline int PROPERTY_COUNT_OPERATION(NAME)() const						\
		{																		\
			return PROPERTY_GET_ACCESSOR (NAME)().size();						\
		}
