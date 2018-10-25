#ifndef STL_UTILS_H
#define STL_UTILS_H


//!
//! Check if a container contains an item.
//!
//! @param container
//!		The container.
//!
//! @param element
//!		The element.
//!
//! @return
//!		Returns true if the item is in the container, false otherwise.
//!
template< typename ContainerType, typename Type >
bool Contains(const ContainerType & container, const Type & element)
{
	return std::find(container.begin(), container.end(), element) != container.end();
}

//!
//! Remove all occurence of an item from a container.
//!
//! @param container
//!		The container.
//!
//! @param item
//!		The item to remove.
//!
template< typename ContainerType, typename Type >
void Remove(ContainerType & container, const Type & item)
{
	auto end = std::remove(container.begin(), container.end(), item);
	if (end != container.end())
	{
		container.erase(end, container.end());
	}
}

//!
//! Get the index of an element in a container.
//!
//! @param container
//!		The container.
//!
//! @param element
//!		The element.
//!
//! @return
//!		The 0 based index of the element in the list if found, -1 otherwise.
//!
template< typename ContainerType, typename Type >
int IndexOf(const ContainerType & container, const Type & element)
{
	auto iterator = std::find(container.begin(), container.end(), element);
	return iterator == container.end() ? -1 : int(std::distance(container.begin(), iterator));
}

//!
//! Sort a container using a functor.
//!
//! @param container
//!		The container.
//!
//! @param functor
//!		The functor. Can be a lambda, std::function, etc.
//!
template< typename ContainerType, typename Functor >
void Sort(ContainerType & container, const Functor & functor)
{
	std::sort(container.begin(), container.end(), functor);
}

//!
//! Sleep the current thread for a given number of micro seconds
//!
inline void SleepForUs(int us)
{
	std::this_thread::sleep_for(std::chrono::microseconds(us));
}

//!
//! Sleep the current thread for a given number of milli seconds
//!
inline void SleepForMs(int ms)
{
	std::this_thread::sleep_for(std::chrono::milliseconds(ms));
}


#endif // STL_UTILS_H
