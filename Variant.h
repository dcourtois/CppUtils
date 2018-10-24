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

	//! string constructor
	inline Variant(const char * value)
		: m_Type(Type::String)
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

	//! pointer constructor
	inline Variant(const void * value)
		: m_Type(Type::Void)
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
	inline Variant & operator = (const char * other)
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
	template< typename T > inline bool operator == (T other) const
	{
		return static_cast< T >(*this) == other;
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
//! Define a vector of variants
//!
typedef std::vector< Variant > Variants;


#endif // VARIANT_H
