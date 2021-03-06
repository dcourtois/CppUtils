#ifndef SETTINGS_H
#define SETTINGS_H


#include "Variant.h"

#include <fstream>


//!
//! Handles saving and loading settings.
//!
class Settings
{

	//! The current version. Upgrade this each time the binary compatibility
	//! with the previous settings changes.
	static constexpr int VERSION = 2;

	//! Setting structure
	struct Setting
	{
		//! The settings values
		Variants Values;

		//! This is used to avoid writting settings that are no longer used
		bool Used;
	};

public:

	//!
	//! Consructor
	//!
	//! @param filename
	//!		In which file the settings are saved / loaded from.
	//!
	inline Settings(const std::string & filename)
		: m_FileName(filename)
		, m_Disabled(false)
	{
	}

	//!
	//! Load the settings
	//!
	inline void Load(void)
	{
		// open for reading in binary format
		std::ifstream file(m_FileName.c_str(), std::ios::binary);
		if (file.is_open() == false)
		{
			return;
		}

		// check the version
		int version = 0;
		file.read(reinterpret_cast< char * >(&version), sizeof(version));
		if (version != VERSION)
		{
			return;
		}

		// load settings
		while(file.eof() == false)
		{
			// name
			size_t size;
			file.read(reinterpret_cast< char * >(&size), sizeof(size_t));
			std::string name(size, 0);
			file.read(&name[0], size);

			// value count
			file.read(reinterpret_cast< char * >(&size), sizeof(size_t));

			// values
			Variants values;
			for (size_t i = 0; i < size; ++i)
			{
				Variant variant;
				variant.Deserialize(file);
				values.push_back(std::move(variant));
			}

			// add it
			m_Settings[name] = { values, false };
		}

		// close file
		file.close();
	}

	//!
	//! Save the settings
	//!
	inline void Save(void)
	{
		// do not save if disabled
		if (m_Disabled == true)
		{
			return;
		}

		// open for writing in binary format
		std::ofstream file(m_FileName.c_str(), std::ios::binary);
		assert(file.is_open());

		// write the version
		int version = VERSION;
		file.write(reinterpret_cast< char * >(&version), sizeof(version));

		// write settings
		for (auto i = m_Settings.begin(), iend = m_Settings.end(); i != iend; ++i)
		{
			// ignore unused settings
			if (i->second.Used == true)
			{
				// name
				size_t size = i->first.size();
				file.write(reinterpret_cast< char * >(&size), sizeof(size_t));
				file.write(i->first.data(), size);

				// number of values
				size = i->second.Values.size();
				file.write(reinterpret_cast< char * >(&size), sizeof(size_t));

				// values
				for (const Variant & value : i->second.Values)
				{
					value.Serialize(file);
				}
			}
		}

		// close file
		file.close();
	}

	//!
	//! Clear all settings
	//!
	inline void Clear(void)
	{
		m_Settings.clear();
	}


	//!
	//! Generic set method.
	//!
	inline void Set(const std::string & name, const Variants & values)
	{
		this->GetSetting(name).Values = values;
	}

	//!
	//! Generic get method.
	//!
	inline const Variants & Get(const std::string & name, const Variants & def)
	{
		Setting & setting = this->GetSetting(name);
		return Compatible(setting.Values, def) == true ? setting.Values : def;
	}

	//!
	//! Templated read method.
	//!
	//! It uses VariantTypeTraits to read from a settings variants list,
	//! so by default it supports types supported by Variants, and you can specialize
	//! VariantTypeTraits to add support to your custom types.
	//!
	template< typename Type > inline Type Get(const std::string & name, const Type & def)
	{
		return m_Disabled == false ? VariantTypeTraits< Type >::FromVariants(this->GetSetting(name).Values, def) : def;
	}

	//!
	//! Templated write method.
	//!
	//! @see Get(const std::string &, const Type &)
	//!
	template< typename Type > inline void Set(const std::string & name, const Type & value)
	{
		this->Set(name, VariantTypeTraits< Type >::ToVariants(value));
	}

private:
	//!
	//! Get a setting by name. If the setting doesn't exist, it will be created.
	//!
	inline Setting & GetSetting(const std::string & name)
	{
		Setting & setting = m_Settings[name];
		setting.Used = true;
		return setting;
	}

	//!
	//! Check if 2 variant lists are compatible.
	//!
	inline bool Compatible(const Variants & left, const Variants & right)
	{
		if (left.size() != right.size())
		{
			return false;
		}
		for (size_t i = 0, iend = left.size(); i < iend; ++i)
		{
			if (left[i].GetType() != right[i].GetType())
			{
				return false;
			}
		}
		return true;
	}

	//! Full path to the file where the settings are stored
	std::string m_FileName;

	//! The current settings
	std::unordered_map< std::string, Setting > m_Settings;

	//! Disabled
	bool m_Disabled;

};


#endif // SETTINGS_H
