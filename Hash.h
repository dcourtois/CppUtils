#ifndef HASH_H
#define HASH_H


#include <string>


namespace Hash
{

	//!
	//! Hash a string using Bob Jenkin's "One-at-a-Time" algorithm.
	//!
	inline uint32_t Jenkins(const char * data, size_t size)
	{
		uint32_t key = 0;
		for (size_t i = 0; i < size; i++)
		{
			key += static_cast< uint32_t >(data[i]);
			key += (key << 10);
			key ^= (key >> 6);
		}
		key += (key << 3);
		key ^= (key >> 11);
		key += (key << 15);
		return key;
	}

	//!
	//! Hash a string using Bob Jenkin's "One-at-a-Time" algorithm.
	//!
	inline uint32_t Jenkins(const std::string & string)
	{
		return Jenkins(string.data(), string.size());
	}

	//!
	//! Helper used by Combine
	//!
	constexpr uint32_t Rotl(uint32_t x, uint32_t r)
	{
		return (x << r) | (x >> (32 - r));
	}

	//!
	//! Helper used to combine 2 hash keys.
	//!
	//! @note
	//!		Implementation is based on Boost's hash_combine one.
	//!
	constexpr uint32_t Combine(uint32_t key, uint32_t hash)
	{
		return Rotl(key ^ (Rotl(hash * 0xcc9e2d51, 15) * 0x1b873593), 13) * 5 + 0xe6546b64;
	}

	//!
	//! Helper to combine more than 2 values
	//!
	template< typename U, typename V, typename ...W > constexpr uint32_t Combine(U key, V hash, W... hashes)
	{
		return Combine(Combine(key, hash), hashes...);
	}

	//!
	//! Implementation detail namespace for static (compile time) hash
	//!
	namespace Detail
	{

		// helpers to implement the key operations using constexpr
		// note: the casts are to avoid static analysis + compile time warnings
		// about integer overflow and downcastings
		constexpr uint32_t A(uint32_t key) { return static_cast< uint32_t >(size_t(key) + (key << 10)); }
		constexpr uint32_t B(uint32_t key) { return static_cast< uint32_t >(size_t(key) ^ (key >> 6)); }
		constexpr uint32_t C(uint32_t key) { return static_cast< uint32_t >(size_t(key) + (key << 3)); }
		constexpr uint32_t D(uint32_t key) { return static_cast< uint32_t >(size_t(key) ^ (key >> 11)); }
		constexpr uint32_t E(uint32_t key) { return static_cast< uint32_t >(size_t(key) + (key << 15)); }

		//! recursive templated constexpr that hashes 1 character and add it to the hash of the next
		//! one. The recusion is stopped at the first character by the specialization Detail::Jenkins< size_t(-1) >
		template< size_t index >
		constexpr uint32_t Jenkins(const char * string)
		{
			return B(A(Jenkins< index - 1 >(string) + string[index]));
		}

		//! specialization of Detail::Jenkins template to stop the recursion at the first character
		template<>
		constexpr uint32_t Jenkins< size_t(-1) >(const char *)
		{
			return 0;
		}

	} // namespace Detail


	//!
	//! Compile-time implementation of Jenkins. Do not use directly, prefer using
	//! @a HASH macro.
	//!
	template< size_t size >
	constexpr uint32_t Jenkins(const char * string)
	{
		return Detail::E(Detail::D(Detail::C(Detail::Jenkins< size - 1 >(string))));
	}

	//!
	//! @def HASH(value)
	//!
	//! Creates a compile-time hash of the @a value string. Those hashes can be used
	//! as case values in a switch. For instance:
	//!
	//! ```cpp
	//! switch (Hash::Jenkins(str.data(), str.size()))
	//! {
	//! 	case HASH("foo"):
	//! 		break;
	//! 
	//! 	case HASH("bar"):
	//! 		break;
	//! }
	//! ```
	//!
	//! @note
	//!		Compile time implementation of Hash::Jenkins
	//!
	#define HASH(value) Hash::Jenkins< sizeof(value) - 1 >(value)


} // namespace Hash


#endif // HASH_H
