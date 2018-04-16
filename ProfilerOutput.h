#ifndef PROFILER_OUTPUT_H
#define PROFILER_OUTPUT_H


#include <string>
#include <mutex>
#include <fstream>
#include <algorithm>

#include "./Profiler.h"


namespace Profiler
{
	namespace Output
	{

		//!
		//! Output the results of a profiling session to a JSON file that can
		//! then be loaded by using the "chrome://tracing" utility of Chrome.
		//!
		//! See Trace Event Format : https://docs.google.com/document/d/1CvAClvFfyA5R-PhYUmn5OOQtYMH4h6I0nSsKchNAySU/preview
		//!
		void ChromeTracing(const std::string & filename)
		{
			// first, ensure that nobody else is modifying the profiling data
			std::lock_guard< std::mutex > lockMarkers(MarkerMutex);
			std::lock_guard< std::mutex > lockScopes(ScopeMutex);

			// open the output file
			std::ofstream file(filename);
			file << "[";

			// and write data
			bool first = true;
			for (const auto & marker : Markers)
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

			// close
			file << std::endl << "]" << std::endl;
			file.close();
		}

	} // namespace Output
} // namespace Profiler


#endif // PROFILER_OUTPUT_H
