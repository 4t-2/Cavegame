#pragma once

#include <list>

template<typename T>
class Environment
{
	public:
		std::list<T> entityList;

		Environment()
		{

		}

		T* addEntity()
		{
			entityList.push_back();
		
			return &entityList.back();
		}
};
