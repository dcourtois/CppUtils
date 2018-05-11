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
		//! Clamp to n decimals
		//!
		inline String GetDouble(double value, int decimals)
		{
			String string = std::to_string(value);
			auto dot = string.find('.');
			if (dot == std::string::npos)
			{
				return string;
			}
			size_t numDecimals = string.size() - 1 - dot;
			return numDecimals > decimals ? string.substr(0, dot + decimals + 1) : string;
		}

		//!
		//! Get a more readable time from nanoseconds
		//!
		inline String GetReadableTime(uint64_t nanoseconds)
		{
			if (nanoseconds < 1000)
			{
				return std::to_string(nanoseconds) + " ns";
			}
			else if (nanoseconds < 1000000)
			{
				return GetDouble(nanoseconds / 1000.0, 2) + " us";
			}
			else if (nanoseconds < 1000000000)
			{
				return GetDouble(nanoseconds / 1000000.0, 2) + " ms";
			}
			else
			{
				return GetDouble(nanoseconds / 1000000000.0, 2) + " s";
			}
		}

		//!
		//! Raw output
		//!
		inline void Raw(const String & filename)
		{
			// first, ensure that nobody else is modifying the profiling data
			ScopedLock< Mutex > lockMarkers(MarkerMutex);
			ScopedLock< Mutex > lockScopes(ScopeMutex);

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
		//! Comma-separated values
		//!
		inline void CommaSeparatedValues(const String & filename)
		{
			// first, ensure that nobody else is modifying the profiling data
			ScopedLock< Mutex > lockMarkers(MarkerMutex);
			ScopedLock< Mutex > lockScopes(ScopeMutex);

			// open the output file
			std::ofstream file(filename, std::ios::binary | std::ios::out);

			// extract data from the markers
			std::unordered_map< ThreadID, std::pair< TimePoint, TimePoint > > timeRanges;
			TimePoint min = GetCurrentTime();
			TimePoint max;
			std::vector< uint64_t > inclusive(Scopes.size(), 0);
			std::vector< int64_t > exclusive(Scopes.size(), 0);
			std::vector< uint64_t > counts(Scopes.size(), 0);
			for (const auto * markers : MarkerLists)
			{
				for (const auto & marker : *markers)
				{
					// update the per-thread time ranges
					auto range = timeRanges.find(marker.Thread);
					if (range == timeRanges.end())
					{
						timeRanges[marker.Thread] = std::make_pair(marker.Start, marker.End);
					}
					else
					{
						if (range->second.first > marker.Start)
						{
							range->second.first = marker.Start;
						}
						if (range->second.second < marker.End)
						{
							range->second.second = marker.End;
						}
					}

					// global time range
					min = std::min(min, marker.Start);
					max = std::max(max, marker.End);

					// update the exec time and counts
					uint64_t ns = GetNanoSeconds(marker.Start, marker.End);
					inclusive[marker.Scope] += ns;
					exclusive[marker.Scope] += ns;
					counts[marker.Scope] += 1;
					if (marker.ParentScope != static_cast<ScopeID>(-1))
					{
						exclusive[marker.ParentScope] -= ns;
					}
				}
			}

			// get the total execution time
			uint64_t execTime = 0;
			for (const auto & range : timeRanges)
			{
				execTime += GetNanoSeconds(range.second.first, range.second.second);
			}

			// output summary
			file << "name;counts;inclusive total;exclusive total;inclusive average;exclusive average;inclusive percentage;exclusive percentage" << std::endl;
			for (size_t i = 0, iend = Scopes.size(); i < iend; ++i)
			{
				file << Scopes[i].Name <<
					";" << counts[i] <<
					";" << GetReadableTime(inclusive[i]) <<
					";" << GetReadableTime(exclusive[i]) <<
					";" << GetReadableTime(inclusive[i] / counts[i]) <<
					";" << GetReadableTime(exclusive[i] / counts[i]) <<
					";" << GetDouble(double(inclusive[i]) / double(execTime), 2) << " %" <<
					";" << GetDouble(double(exclusive[i]) / double(execTime), 2) << " %" <<
					std::endl;
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
			ScopedLock< Mutex > lockMarkers(MarkerMutex);
			ScopedLock< Mutex > lockScopes(ScopeMutex);

			// open the output file
			std::ofstream file(filename);
			file << "[";

			// and write data
			bool first = true;
			for (const auto * markers : MarkerLists)
			{
				for (const auto & marker : *markers)
				{
					const ScopeData & scope = Scopes[marker.Scope];
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
					file << "\"tid\": " << marker.Thread << ", ";
					file << "\"ts\": " << GetMicroSeconds(Start, marker.Start) << ", ";
					file << "\"dur\": " << GetMicroSeconds(marker.Start, marker.End) << ", ";
					file << "\"args\": { \"filename\": \"" << scopeFilename << "\", \"line\": " << scope.Line << " } }";
				}
			}

			// close
			file << std::endl << "]" << std::endl;
		}

	} // namespace Output
} // namespace Profiler


#endif // PROFILER_ENABLE == 1

#endif // PROFILER_OUTPUT_H
