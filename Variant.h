#ifndef VARIANT_H
#define VARIANT_H


#include <cassert>
#include <cstdlib>
#include <new>
#include <string>


//!
//! Variant class. It can store numerical values , booleans or std::string's and convert
//! from / to any of those types.
//!
class Variant
{

public:

	//! the type of value
	enum class Type
	{
		String = 0,		//!< std::string
		Double,			//!< double
		Signed,			//!< int64_t
		Unsigned,		//!< uint64_t
		Bool,			//!< bool
		Void,			//!< pointer
		None,			//!< not set
	};

	//! default empty constructor
	inline Variant(void)
		: m_Type(Type::None)
		, m_Data{}
	{
	}

	//! pointer constructor
	template< typename T > inline Variant(const T * value)
		: m_Type(Type::Void)
		, m_Data(value)
	{
	}

	//! string constructor
	inline Variant(const std::string & value)
		: m_Type(Type::String)
		, m_Data(value)
	{
	}

	//! string constructor
	inline Variant(std::string && value)
		: m_Type(Type::String)
		, m_Data(std::move(value))
	{
	}

	//! double constructor
	inline Variant(double value)
		: m_Type(Type::Double)
		, m_Data(value)
	{
	}

	//! double constructor
	inline Variant(float value)
		: m_Type(Type::Double)
		, m_Data(static_cast< double >(value))
	{
	}

	//! integer constructor
	inline Variant(int64_t value)
		: m_Type(Type::Signed)
		, m_Data(value)
	{
	}

	//! integer constructor
	inline Variant(int value)
		: m_Type(Type::Signed)
		, m_Data(static_cast< int64_t >(value))
	{
	}

	//! unsigned integer constructor
	inline Variant(uint64_t value)
		: m_Type(Type::Unsigned)
		, m_Data(value)
	{
	}

	//! unsigned integer constructor
	inline Variant(unsigned int value)
		: m_Type(Type::Unsigned)
		, m_Data(static_cast< uint64_t >(value))
	{
	}

	//! bool constructor
	inline Variant(bool value)
		: m_Type(Type::Bool)
		, m_Data(value)
	{
	}

	//! copy constructor
	inline Variant(const Variant & other)
		: m_Type(other.m_Type)
	{
		switch (m_Type)
		{
			case Type::String:		new (&m_Data.s) std::string(other.m_Data.s);	break;
			case Type::Double:		m_Data.d = other.m_Data.d;						break;
			case Type::Signed:		m_Data.i = other.m_Data.i;						break;
			case Type::Unsigned:	m_Data.ui = other.m_Data.ui;					break;
			case Type::Bool:		m_Data.b = other.m_Data.b;						break;
			case Type::Void:		m_Data.v = other.m_Data.v;						break;
			case Type::None:		break;
		}
	}

	//! move constructor
	inline Variant(Variant && other)
		: m_Type(other.m_Type)
	{
		switch (m_Type)
		{
			case Type::String:		new (&m_Data.s) std::string(std::move(other.m_Data.s));	break;
			case Type::Double:		m_Data.d = other.m_Data.d;								break;
			case Type::Signed:		m_Data.i = other.m_Data.i;								break;
			case Type::Unsigned:	m_Data.ui = other.m_Data.ui;							break;
			case Type::Bool:		m_Data.b = other.m_Data.b;								break;
			case Type::Void:		m_Data.v = other.m_Data.v;								break;
			case Type::None:		break;
		}
	}

	//! destructor
	inline ~Variant(void)
	{
		if (m_Type == Type::String)
		{
			// note : `m_Data.s.~std::string();` is invalid, thus the following
			using namespace std;
			m_Data.s.~string();
		}
	}

	//! assignment operator
	inline Variant & operator = (const Variant & other)
	{
		assert(this != &other);
		this->~Variant();
		new (this) Variant(other);
		return *this;
	}

	//! assignment operator
	inline Variant & operator = (Variant && other)
	{
		assert(this != &other);
		this->~Variant();
		new (this) Variant(std::move(other));
		return *this;
	}

	//! assignment operator
	template< typename T > inline Variant & operator = (const T * other)
	{
		this->~Variant();
		new (this) Variant(other);
		return *this;
	}

	//! assignment operator
	inline Variant & operator = (const std::string & other)
	{
		this->~Variant();
		new (this) Variant(other);
		return *this;
	}

	//! assignment operator
	inline Variant & operator = (std::string && other)
	{
		this->~Variant();
		new (this) Variant(std::move(other));
		return *this;
	}

	//! template assignment operator
	template< typename T > inline Variant & operator = (T other)
	{
		this->~Variant();
		new (this) Variant(other);
		return *this;
	}

	//! equality operator
	inline bool operator == (const Variant & other) const
	{
		if (m_Type != other.m_Type)
		{
			return false;
		}
		switch (m_Type)
		{
			case Type::String:		return m_Data.s == other.m_Data.s;
			case Type::Double:		return m_Data.d == other.m_Data.d;
			case Type::Signed:		return m_Data.i == other.m_Data.i;
			case Type::Unsigned:	return m_Data.ui == other.m_Data.ui;
			case Type::Bool:		return m_Data.b == other.m_Data.b;
			case Type::Void:		return m_Data.v == other.m_Data.v;
			default:				return true;
		}
	}

	//! equality operator
	template< typename T > inline bool operator == (T other) const
	{
		return static_cast< T >(*this) == other;
	}

	//! inequality operator
	inline bool operator != (const Variant & other) const
	{
		if (m_Type != other.m_Type)
		{
			return true;
		}
		switch (m_Type)
		{
			case Type::String:		return m_Data.s != other.m_Data.s;
			case Type::Double:		return m_Data.d != other.m_Data.d;
			case Type::Signed:		return m_Data.i != other.m_Data.i;
			case Type::Unsigned:	return m_Data.ui != other.m_Data.ui;
			case Type::Bool:		return m_Data.b != other.m_Data.b;
			case Type::Void:		return m_Data.v != other.m_Data.v;
			default:				return false;
		}
	}

	//! inequality operator
	template< typename T > inline bool operator != (T other) const
	{
		return static_cast< T >(*this) != other;
	}

	//! reset value to none
	inline void Reset(void)
	{
		this->~Variant();
		m_Type = Type::None;
	}

	//! get the type of the value
	inline Type GetType(void) const
	{
		return m_Type;
	}

	//! returns true if the type is numerical (either Signed, Unsigned or Double)
	inline bool IsTypeNum(void) const
	{
		return m_Type == Type::Signed || m_Type == Type::Unsigned || m_Type == Type::Double;
	}

	//! returns true if the type is integer (either Signed or Unsigned)
	inline bool IsTypeInt(void) const
	{
		return m_Type == Type::Signed || m_Type == Type::Unsigned;
	}

	//! convert to std::string
	inline operator std::string (void) const
	{
		switch (m_Type)
		{
			case Type::String:		return m_Data.s;
			case Type::Double:		return std::to_string(m_Data.d);
			case Type::Signed:		return std::to_string(m_Data.i);
			case Type::Unsigned:	return std::to_string(m_Data.ui);
			case Type::Bool:		return m_Data.b == true ? "1" : "0";
			case Type::Void:		return std::to_string(reinterpret_cast< size_t >(m_Data.v));
			default:				return "";
		}
	}

	//! convert to double
	inline operator double (void) const
	{
		switch (m_Type)
		{
			case Type::String:		return 0.0;
			case Type::Double:		return m_Data.d;
			case Type::Signed:		return static_cast< double >(m_Data.i);
			case Type::Unsigned:	return static_cast< double >(m_Data.ui);
			case Type::Bool:		return m_Data.b == true ? 1.0 : 0.0;
			case Type::Void:		assert(false && "unsupported conversion"); return 0.0;
			default:				return 0.0;
		}
	}

	//! convert to float
	inline operator float (void) const
	{
		return static_cast< float >(static_cast< double >(*this));
	}

	//! convert to int64_t
	inline operator int64_t (void) const
	{
		switch (m_Type)
		{
			case Type::String:		return std::atoll(m_Data.s.c_str());
			case Type::Double:		return static_cast< int64_t >(m_Data.d);
			case Type::Signed:		return m_Data.i;
			case Type::Unsigned:	return static_cast< int64_t >(m_Data.ui);
			case Type::Bool:		return m_Data.b == true ? 1 : 0;
			case Type::Void:		return reinterpret_cast< int64_t >(m_Data.v);
			default:				return 0;
		}
	}

	//! convert to int
	inline operator int (void) const
	{
		return static_cast< int >(static_cast< int64_t >(*this));
	}

	//! convert to uint64_t
	inline operator uint64_t (void) const
	{
		switch (m_Type)
		{
			case Type::String:		return std::atoll(m_Data.s.c_str());
			case Type::Double:		return static_cast< int >(m_Data.d);
			case Type::Signed:		return static_cast< int >(m_Data.i);
			case Type::Unsigned:	return m_Data.ui;
			case Type::Bool:		return m_Data.b == true ? 1 : 0;
			case Type::Void:		return reinterpret_cast< uint64_t >(m_Data.v);
			default:				return 0;
		}
	}

	//! convert to unsigned int
	inline operator unsigned int (void) const
	{
		return static_cast< unsigned int >(static_cast< uint64_t >(*this));
	}

	//! convert to bool
	inline operator bool (void) const
	{
		switch (m_Type)
		{
			case Type::String:		return std::atoll(m_Data.s.c_str()) != 0;
			case Type::Double:		return m_Data.d != 0.0;
			case Type::Signed:		return m_Data.i != 0;
			case Type::Unsigned:	return m_Data.ui != 0;
			case Type::Bool:		return m_Data.b;
			case Type::Void:		return m_Data.v != nullptr;
			default:				return false;
		}
	}

	//! convert to const void *
	inline operator const void * (void) const
	{
		switch (m_Type)
		{
			case Type::String:		return m_Data.s.c_str();
			case Type::Double:		return reinterpret_cast< const void * >(*reinterpret_cast< const uint64_t * >(&m_Data.d));
			case Type::Signed:		return reinterpret_cast< const void * >(m_Data.i);
			case Type::Unsigned:	return reinterpret_cast< const void * >(m_Data.i);
			case Type::Bool:		return reinterpret_cast< const void * >(static_cast< uint64_t >(m_Data.b == true ? 1 : 0));
			case Type::Void:		return m_Data.v;
			default:				return nullptr;
		}
	}

	//! convert to a pointer type
	template< typename T > inline T * ToPointer(void) const
	{
		assert(m_Type == Type::Void && "only Void type variants can be cast to a pointer type.");
		return const_cast< T * >(reinterpret_cast< const T * >(m_Data.v));
	}

	//! serialize the variant to a binary stream
	inline void Serialize(std::ostream & output) const
	{
		// the type
		output.write(reinterpret_cast< const char * >(&m_Type), sizeof(m_Type));

		// the value
		switch (m_Type)
		{
			case Type::String:
			{
				size_t size = m_Data.s.size();
				output.write(reinterpret_cast< const char * >(&size), sizeof(size_t));
				output.write(m_Data.s.data(), size);
				break;
			}

			case Type::Double:
				output.write(reinterpret_cast< const char * >(&m_Data.d), sizeof(double));
				break;

			case Type::Signed:
				output.write(reinterpret_cast< const char * >(&m_Data.i), sizeof(int64_t));
				break;

			case Type::Unsigned:
				output.write(reinterpret_cast< const char * >(&m_Data.ui), sizeof(uint64_t));
				break;

			case Type::Bool:
				output.write(reinterpret_cast< const char * >(&m_Data.b), sizeof(bool));
				break;

			case Type::Void:
				output.write(reinterpret_cast< const char * >(&m_Data.v), sizeof(void *));
				break;

			default:
				break;
		}
	}

	//! deserialize the variant from a binary stream
	inline void Deserialize(std::istream & output)
	{
		// reset
		this->Reset();

		// the type
		output.read(reinterpret_cast< char * >(&m_Type), sizeof(m_Type));

		// the value
		switch (m_Type)
		{
			case Type::String:
			{
				size_t size = 0;
				output.read(reinterpret_cast< char * >(&size), sizeof(size_t));
				m_Data.s.resize(size);
				output.read(&m_Data.s[0], size);
				break;
			}

			case Type::Double:
				output.read(reinterpret_cast< char * >(&m_Data.d), sizeof(double));
				break;

			case Type::Signed:
				output.read(reinterpret_cast< char * >(&m_Data.i), sizeof(int64_t));
				break;

			case Type::Unsigned:
				output.read(reinterpret_cast< char * >(&m_Data.ui), sizeof(uint64_t));
				break;

			case Type::Bool:
				output.read(reinterpret_cast< char * >(&m_Data.b), sizeof(bool));
				break;

			case Type::Void:
				output.read(reinterpret_cast< char * >(&m_Data.v), sizeof(void *));
				break;

			default:
				break;
		}
	}

private:

	//! the type of the value
	Type m_Type;

	//! the actual value
	union Data
	{
		//! string data
		std::string s;

		// double data
		double d;

		// integer data
		int64_t i;

		// unsigned integer data
		uint64_t ui;

		// bool data
		bool b;

		// pointer data
		const void * v;

		//! default empty constructor
		inline Data(void) {}

		//! string constructor
		inline Data(const char * value) : s(value) {}

		//! string constructor
		inline Data(const std::string & value) : s(value) {}

		//! string constructor
		inline Data(std::string && value) : s(std::move(value)) {}

		//! double construct
		inline Data(double value) : d(value) {}

		//! integer construct
		inline Data(int64_t value) : i(value) {}

		//! unsigned integer construct
		inline Data(uint64_t value) : ui(value) {}

		//! bool construct
		inline Data(bool value) : b(value) {}

		//! pointer construct
		inline Data(const void * value) : v(value) {}

		//! default empty destructor
		inline ~Data(void) {}

	} m_Data;

};

//!
//! Specialization of the pointer constructor for strings.
//!
//! @note
//!		Apparently this cannot be done inside the class scope.
//!
template<> inline Variant::Variant(const char * value)
	: m_Type(Type::String)
	, m_Data(value)
{
}


//!
//! Define a vector of variants
//!
typedef std::vector< Variant > Variants;

//!
//! This is used to convert data between concrete types and variants
//! You can specialize this for your types to let the Settings class
//! handle them.
//!
//! @code{.cpp}
//!
//!	// specialization for a Vector4 type
//!	template< > struct VariantTypeTraits< Vector4 >
//!	{
//!		static inline Vector4 FromVariants(const Variants & variants, const Vector4 & def)
//!		{
//!			return variants.size() == 4 ? Vector4(variants[0], variants[1], variants[2], variants[3]) : def;
//!		}
//!		static inline Variants ToVariants(const Vector4 & value)
//!		{
//!			return { value[0], value[1], value[2], value[3] };
//!		}
//!	};
//!
//!	// then you can use Vector4 settings
//!	Settings::Instance().Set("some_vector", Vector4(0.0, 1.0, 2.0, 3.0));
//!
//! @endcode
//!
template< typename Type >
struct VariantTypeTraits
{
	//!
	//! Convert from a list of variant to a concrete type.
	//!
	//! @param variants
	//!		The list of variant.
	//!
	//! @param def
	//!		Default value which should be returned in case the variants are not
	//!		compatible with the given type.
	//!
	//! @return
	//!		Either the value read from @p variants or @p def.
	//!
	static inline Type FromVariants(const Variants & variants, Type def)
	{
		return variants.size() == 1 ? static_cast< Type >(variants[0]) : def;
	}

	//!
	//! Convert from a concrete type to a list of variant.
	//!
	//! @param value
	//!		The value to convert to Variants
	//!
	//! @return
	//!		The list of variant describing the value.
	//!
	static inline Variants ToVariants(Type value)
	{
		return { value };
	}
};

//!
//! Closing call of the variadic ToVariants function.
//!
template< typename T > inline Variants ToVariants(T value)
{
	return VariantTypeTraits< T >::ToVariants(value);
}

//!
//! Helper used to convert many values into variant lists, and pack them all into a single
//! variant list.
//!
template< typename T, typename ...Ts > inline Variants ToVariants(T first, Ts... zeRest)
{
	Variants variants = VariantTypeTraits< T >::ToVariants(first);
	for (Variant & variant : ToVariants(zeRest...))
	{
		variants.push_back(std::move(variant));
	}
	return variants;
}

//!
//! Helper to compare 2 lists of variants
//!
inline bool operator == (const Variants & left, const Variants & right)
{
	if (left.size() != right.size())
	{
		return false;
	}
	for (auto l = left.begin(), r = right.begin(), lend = left.end(), rend = right.end(); l != lend; ++l, ++r)
	{
		if (*l != *r)
		{
			return false;
		}
	}
	return true;
}

//!
//! Helper to compare 2 lists of variants
//!
inline bool operator != (const Variants & left, const Variants & right)
{
	if (left.size() != right.size())
	{
		return true;
	}
	for (auto l = left.begin(), r = right.begin(), lend = left.end(), rend = right.end(); l != lend; ++l, ++r)
	{
		if (*l != *r)
		{
			return true;
		}
	}
	return false;
}


#endif // VARIANT_H
