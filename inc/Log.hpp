#pragma once

#include <chrono>
#include <functional>
#include <iostream>
#include <mutex>
#include <string>
#include <vector>

#define MAXLOGS 10

class Log
{
	public:

		class Entry
		{
			public:
				std::chrono::system_clock::time_point time;
				std::string							  data;
		};
		static void addLog(std::string data)
		{
			std::lock_guard gaurd(lock);
			auto now = std::chrono::system_clock::now();
			entryList.emplace_back(now, data);
			std::cout <<  now << " : " << data << '\n';
			
			if (entryList.size() > MAXLOGS)
			{
				entryList.erase(entryList.begin());
			}
		}

		static void clear()
		{
			std::lock_guard gaurd(lock);
			entryList.clear();
		}

		static void iterate(std::function<void(Entry&)> func)
		{
			std::lock_guard gaurd(lock);

			for(auto &e: entryList)
			{
				func(e);
			}
		}
	private:
		inline static std::vector<Entry> entryList;
		inline static std::mutex lock;
};
