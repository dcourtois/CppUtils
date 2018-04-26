#ifndef PROFILER_OUTPUT_H
#define PROFILER_OUTPUT_H


#if PROFILER_ENABLE == 1


#include <string>
#include <mutex>
#include <fstream>
#include <algorithm>


namespace Profiler
{
	namespace Output
	{

		//!
		//! Generic binary write
		//!
		inline void Write(std::ofstream & file, const void * data, size_t size)
		{
			file.write(reinterpret_cast< const char * >(data), size);
		}

		//!
		//! Utility to write binary values
		//!
		template< typename T > inline void Write(std::ofstream & file, const T & data)
		{
			file.write(reinterpret_cast< const char * >(&data), sizeof(T));
		}

		//!
		//! Override for strings
		//!
		template<> inline void Write(std::ofstream & file, const std::string & data)
		{
			Write(file, data.size());
			file.write(data.data(), data.size());
		}

		//!
		//! Raw output
		//!
		inline void Raw(const std::string & filename)
		{
			// first, ensure that nobody else is modifying the profiling data
			std::lock_guard< std::mutex > lockMarkers(MarkerMutex);
			std::lock_guard< std::mutex > lockScopes(ScopeMutex);

			// open the output file
			std::ofstream file(filename, std::ios::binary | std::ios::out);

			// Write the start time
			Write(file, Start);

			// write the scopes
			Write(file, Scopes.size());
			for (const auto & scope : Scopes)
			{
				Write(file, scope.Filename);
				Write(file, scope.Name);
				Write(file, scope.Line);
			}

			// write the markers
			Write(file, MarkerLists.size());
			for (const auto * markers : MarkerLists)
			{
				Write(file, markers->size());
				Write(file, markers->data(), markers->size());
			}
		}

		//!
		//! Output the results of a profiling session to a JSON file that can
		//! then be loaded by using the "chrome://tracing" utility of Chrome.
		//!
		//! See Trace Event Format : https://docs.google.com/document/d/1CvAClvFfyA5R-PhYUmn5OOQtYMH4h6I0nSsKchNAySU/preview
		//!
		inline void ChromeTracing(const std::string & filename)
		{
			// first, ensure that nobody else is modifying the profiling data
			std::lock_guard< std::mutex > lockMarkers(MarkerMutex);
			std::lock_guard< std::mutex > lockScopes(ScopeMutex);

			// open the output file
			std::ofstream file(filename);
			file << "[";

			// and write data
			bool first = true;
			for (const auto * markers : MarkerLists)
			{
				for (const auto & marker : *markers)
				{
					const ScopeData & scope = Scopes[marker.ScopeID];
					std::string scopeFilename = scope.Filename;
					std::replace(scopeFilename.begin(), scopeFilename.end(), '\\', '/');
					if (first == false)
					{
						file << "," << std::endl;
					}
					else
					{
						file << std::endl;
						first = false;
					}
					file << "{ \"cat\": \"perf\", \"ph\": \"X\", \"pid\": \"foo\", ";
					file << "\"name\": \"" << scope.Name << "\", ";
					file << "\"tid\": " << marker.ID << ", ";
					file << "\"ts\": " << GetMicroSeconds(Start, marker.Start) << ", ";
					file << "\"dur\": " << GetMicroSeconds(marker.Start, marker.End) << ", ";
					file << "\"args\": { \"filename\": \"" << scopeFilename << "\", \"line\": " << scope.Line << " } }";
				}
			}

			// close
			file << std::endl << "]" << std::endl;
			file.close();
		}

	} // namespace Output
} // namespace Profiler


#endif // PROFILER_ENABLE == 1

#endif // PROFILER_OUTPUT_H
