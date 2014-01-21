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

#ifndef LDFORGE_PROPERTY_H
#define LDFORGE_PROPERTY_H

#define PROPERTY( ACCESS, TYPE, NAME, OPS, WRITETYPE )			\
	private:													\
		TYPE m_##NAME;											\
																\
	public:														\
		inline TYPE const& GET_READ_METHOD( NAME, OPS ) const	\
		{														\
			return m_##NAME; 									\
		}														\
																\
	ACCESS:														\
		DEFINE_WRITE_METHOD_##WRITETYPE( TYPE, NAME )			\
		DEFINE_PROPERTY_##OPS( TYPE, NAME )

#define GET_READ_METHOD( NAME, OPS ) \
	GET_READ_METHOD_##OPS( NAME )

#define GET_READ_METHOD_BOOL_OPS( NAME ) is##NAME()
#define GET_READ_METHOD_NO_OPS( NAME ) get##NAME()
#define GET_READ_METHOD_STR_OPS( NAME ) get##NAME()
#define GET_READ_METHOD_NUM_OPS( NAME ) get##NAME()
#define GET_READ_METHOD_LIST_OPS( NAME ) get##NAME()

#define DEFINE_WRITE_METHOD_STOCK_WRITE( TYPE, NAME )	\
		inline void set##NAME( TYPE const& NAME##_ )	\
		{												\
			m_##NAME = NAME##_;							\
		}

#define DEFINE_WRITE_METHOD_CUSTOM_WRITE( TYPE, NAME )	\
		void set##NAME( TYPE const& NAME##_ );			\

#define DEFINE_WITH_CB( NAME ) void NAME##Changed();
#define DEFINE_NO_CB( NAME )

#define DEFINE_PROPERTY_NO_OPS( TYPE, NAME )

#define DEFINE_PROPERTY_STR_OPS( TYPE, NAME )			\
		void appendTo##NAME( TYPE a )					\
		{												\
			TYPE tmp( m_##NAME );						\
			tmp.append( a );							\
			set##NAME( tmp );							\
		}												\
														\
		void prependTo##NAME( TYPE a )					\
		{												\
			TYPE tmp( m_##NAME );						\
			tmp.prepend( a );							\
			set##NAME( tmp );							\
		}												\
														\
		void replaceIn##NAME( TYPE a, TYPE b )			\
		{												\
			TYPE tmp( m_##NAME );						\
			tmp.replace( a, b );						\
			set##NAME( tmp );							\
		}

#define DEFINE_PROPERTY_NUM_OPS( TYPE, NAME )			\
		inline void increase##NAME( TYPE a = 1 )		\
		{												\
			set##NAME( m_##NAME + a );					\
		}												\
														\
		inline void decrease##NAME( TYPE a = 1 )		\
		{												\
			set##NAME( m_##NAME - a );					\
		}

#define DEFINE_PROPERTY_BOOL_OPS( TYPE, NAME )				\
		inline void toggle##NAME()							\
		{													\
			set##NAME( !m_##NAME );							\
		}

#define DEFINE_PROPERTY_LIST_OPS( TYPE, NAME )				\
		void pushTo##NAME( const TYPE::value_type& a )		\
		{													\
			TYPE tmp( m_##NAME );							\
			tmp.push_back( a );								\
			set##NAME( tmp );								\
		}													\
															\
		void removeFrom##NAME( const TYPE::value_type& a )	\
		{													\
			TYPE tmp( m_##NAME );							\
			tmp.removeOne( a );								\
			set##NAME( tmp );								\
		}													\
															\
		inline void clear##NAME()							\
		{													\
			set##NAME( TYPE() );							\
		}													\
															\
	public:													\
		inline int count##NAME()							\
		{													\
			return get##NAME().size();						\
		}

#endif // LDFORGE_PROPERTY_H